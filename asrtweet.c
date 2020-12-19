// Extract tweets for printing on ASR33
// Expects to be able to run twarc

#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <popt.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <err.h>
#include <mosquitto.h>
#include <ajlparse.h>
#include <ajl.h>

int main(int argc, const char *argv[])
{
   int debug = 0;
   const char *alias = NULL;
   const char *footnote = NULL;
   const char *mqtthostname = NULL;
   const char *mqttusername = NULL;
   const char *mqttpassword = NULL;
   const char *mqttid = NULL;
   const char *mqttcafile = NULL;
   int mqttport = 0;
   const char *tty = NULL;
   int background = 0;
   int idn = 0;
   long long *ids = NULL;
   {                            // POPT
      poptContext optCon;       // context for parsing command-line options
      const struct poptOption optionsTable[] = {
         { "mqtt-hostname", 'h', POPT_ARG_STRING, &mqtthostname, 0, "MQTT hostname", "hostname" },
         { "mqtt-username", 'u', POPT_ARG_STRING, &mqttusername, 0, "MQTT username", "username" },
         { "mqtt-password", 'p', POPT_ARG_STRING, &mqttpassword, 0, "MQTT password", "password" },
         { "mqtt-ca", 'C', POPT_ARG_STRING, &mqttcafile, 0, "MQTT CA", "filename" },
         { "mqtt-port", 0, POPT_ARG_INT, &mqttport, 0, "MQTT port", "port" },
         { "mqtt-id", 0, POPT_ARG_STRING, &mqttid, 0, "MQTT id", "id" },
         { "tty", 0, POPT_ARG_STRING, &tty, 0, "TTY id", "id" },
         { "alias", 'a', POPT_ARG_STRING, &alias, 0, "Alias (1st ID)", "name" },
         { "footnote", 'f', POPT_ARG_STRING, &footnote, 0, "footnote (1st ID)", "text" },
         { "alias", 'a', POPT_ARG_STRING, &alias, 0, "Alias", "name" },
         { "daemon", 'D', POPT_ARG_NONE, &background, 0, "Daemon" },
         { "debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug" },
         POPT_AUTOHELP { }
      };

      optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp(optCon, "[tweet-ids]");

      int c;
      if ((c = poptGetNextOpt(optCon)) < -1)
         errx(1, "%s: %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(c));

      const char *id;
      while ((id = poptGetArg(optCon)))
      {
         ids = realloc(ids, sizeof(*ids) * (++idn));
         if (!(ids[idn - 1] = strtoull(id, NULL, 0)))
            errx(1, "Cannot parse id %s. Use twitter numeric ID", id);
      }
      if (!debug && !idn)
      {
         poptPrintUsage(optCon, stderr, 0);
         errx(1, "Specify twitter IDs to follow");
      }
      poptFreeContext(optCon);
   }
   if (background)
   {
      if (fork())
         return 0;
      daemon(0, 1);
   }

   int e = mosquitto_lib_init();
   if (e)
      errx(1, "MQTT init failed %s", mosquitto_strerror(e));
   struct mosquitto *mqtt = mosquitto_new(mqttid, 1, NULL);
   if (mqttusername)
   {
      e = mosquitto_username_pw_set(mqtt, mqttusername, mqttpassword);
      if (e)
         errx(1, "MQTT auth failed %s", mosquitto_strerror(e));
   }
   if (mqttcafile && (e = mosquitto_tls_set(mqtt, mqttcafile, NULL, NULL, NULL, NULL)))
      warnx("MQTT cert failed (%s) %s", mqttcafile, mosquitto_strerror(e));
   e = mosquitto_connect(mqtt, mqtthostname ? : "localhost", mqttport ? : mqttcafile ? 8883 : 1883, 60);
   if (e)
      errx(1, "MQTT connect failed %s", mosquitto_strerror(e));
   mosquitto_loop_start(mqtt);

   char *topic = NULL;
   if (asprintf(&topic, "command/ASR33/%s/text", tty ? : "*") < 0)
      errx(1, "malloc");

   FILE *i = stdin;
   if (idn)
   {
      char *cmd = NULL;
      size_t l;
      FILE *o = open_memstream(&cmd, &l);
      fprintf(o, "twarc --log /dev/null filter --follow ");
      for (int i = 0; i < idn; i++)
         fprintf(o, "%s%lld", i ? "," : "", ids[i]);
      fclose(o);
      i = popen(cmd, "r");
      if (!i)
         err(1, "failed: %s", cmd);
      if (debug)
         warnx("%s", cmd);
      free(cmd);
   }

   ajl_t p = ajl_read(i);
   j_t j = j_create();
   while (1)
   {
      const char *er = j_recv(j, p);
      if (er && !*er)
         break;
      if (er)
         errx(1, "failed: %s", er);
      if (debug)
         j_err(j_write_pretty(j, stderr));
      long long id = strtoull(j_get(j, "user.id"), NULL, 0);
      int i = 0;
      if (idn)
      {
         for (i = 0; i < idn && id != ids[i]; i++);
         if (i == idn)
            continue;
      }
      unsigned long long tsms = strtoull(j_get(j, "timestamp_ms"), NULL, 0);
      time_t ts = tsms / 1000;
      struct tm tm;
      gmtime_r(&ts, &tm);
      char when[100];
      strftime(when, sizeof(when), "%FT%T", &tm);
      const char *name = (i ? NULL : alias),
          *at = "";
      if (!name)
      {
         name = j_get(j, "user.name");
         if (!name || !*name || strlen(name) > 20 || (*name & 0x80))
         {
            at = "@";
            name = j_get(j, "user.screen_name");
         }
      }
      j_t entities = j_find(j, "extended_tweet.entities") ? : j_find(j, "entities");
      char *in = strdup(j_get(j, "extended_tweet.full_text") ? : j_get(j, "text") ? : "");
      int lenin = strlen(in),
          lenout = lenin + 1;
      char *out = malloc(lenout);
      {                         // WTF is twitter XML coding stuff in JSON, really. De-XML stuff
         int i = 0,
             o = 0;
         while (in[i])
         {
            void replace(int len, const char *new) {
               if (len < 0 || !new)
                  return;
               int newlen = strlen(new);
               if (newlen != len)
                  out = realloc(out, lenout += newlen - len);
               memcpy(out + o, new, newlen);
               o += newlen;
               i += len;
            }
            if (entities)
            {                   // URLS
               j_t u = NULL;
               for (u = j_first(j_find(entities, "urls")); u; u = j_next(u))
               {
                  j_t ind = j_find(u, "indices");
                  if (j_isarray(ind) && j_len(ind) == 2 && atoi(j_val(j_index(ind, 0))) == i)
                  {
                     replace(atoi(j_val(j_index(ind, 1))) - i, j_get(u, "display_url"));
                     break;
                  }
               }
               if (u)
                  continue;     // found
            }
            if (entities)
            {                   // media
               j_t m = NULL;
               for (m = j_first(j_find(entities, "media")); m; m = j_next(m))
               {
                  j_t ind = j_find(m, "indices");
                  if (j_isarray(ind) && j_len(ind) == 2 && atoi(j_val(j_index(ind, 0))) == i)
                  {
                     replace(atoi(j_val(j_index(ind, 1))) - i, "[media]");
                     break;
                  }
               }
               if (m)
                  continue;     // found
            }
            if (!strncmp(in + i, "&amp;", 5))
            {
               replace(5, "&");
               continue;
            }
            if (!strncmp(in + i, "&lt;", 4))
            {
               replace(4, "<");
               continue;
            }
            if (!strncmp(in + i, "&gt;", 4))
            {
               replace(4, ">");
               continue;
            }
            out[o++] = in[i++];
         }
         out[o] = 0;
      }
      free(in);
      const char *place = j_get(j, "place.name");
      if (!strcmp(out, "[media]"))
      {                         // No print printing if just media
         free(out);
         continue;
      }
      // Simple write out
      char *msg = NULL;
      size_t l = 0;
      FILE *o = open_memstream(&msg, &l);
      fprintf(o, "\n");
      fprintf(o, "+++ %s.%03dZ FROM %s%s", when, (int) (tsms % 1000), at, name);
      if (place)
         fprintf(o, " IN %s", place);
      fprintf(o, " +++\n");
      fprintf(o, "%s\n", out);
      if (!i && footnote)
         fprintf(o, "*** %s ***\n", footnote);
      fclose(o);
      int e = mosquitto_publish(mqtt, NULL, topic, l, msg, 0, 0);
      if (e)
         warnx("MQTT publish failed %s (%s) %d bytes", mosquitto_strerror(e), topic, l);
      if (debug)
         fprintf(stderr, "%s", msg);
      free(msg);
      free(out);
   }
   j_free(j);

   return 0;
}
