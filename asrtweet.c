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
   int wrap = 72;
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
         { "wrap", 0, POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &wrap, 0, "Wrap", "Chars" },
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
      unsigned long long tsms = strtoull(j_get(j, "timestamp_ms") ? : "", NULL, 0);
      time_t ts = tsms / 1000;
      struct tm tm;
      gmtime_r(&ts, &tm);
      char when[100] = "";
      if (ts)
      {
         strftime(when, sizeof(when) - 5, "%FT%T", &tm);
         sprintf(when + strlen(when), ".%03dZ", tsms % 1000);
      } else
         strncpy(when, j_get(j, "created_at") ? : "", sizeof(when));
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
      j_t t = j_find(j, "retweeted_status") ? : j;
      j_t entities = j_find(t, "extended_tweet.entities") ? : j_find(t, "entities");
      char *in = strdup(j_get(t, "extended_tweet.full_text") ? : j_get(t, "full_text") ? : j_get(t, "text") ? : "");
      int lenin = strlen(in),
          lenout = lenin + 1;
      char *out = malloc(lenout);
      int text = 0;
      int pos = 0;
      int skip = atoi(j_val(j_index(j_find(t, "extended_tweet.display_text_range") ? : j_find(t, "display_text_range"), 0)) ? : "");;
      {                         // WTF is twitter XML coding stuff in JSON, really. De-XML stuff
         int i = 0,
             o = 0;
         while (in[i])
         {
            if (!skip)
            {
               inline void replace(int len, const char *new) {
                  if (len < 0 || !new)
                     return;
                  int newlen = strlen(new);
                  if (newlen != len)
                     out = realloc(out, lenout += newlen - len);
                  memcpy(out + o, new, newlen);
                  o += newlen;
                  for (int q = 0; q < len; q++)
                     if ((in[i + q] & 0xC0) != 0x80)
                        skip++;
               }
               inline int check(const char *from, const char *to) {
                  int len = strlen(from);
                  if (strncmp(in + i, from, len))
                     return 0;  // Not found
                  replace(len, to);     // Replace
                  return 1;
               }
               if (entities)
               {                // User mentions
                  j_t u = NULL;
                  for (u = j_first(j_find(entities, "user_mentions")); u; u = j_next(u))
                  {
                     j_t ind = j_find(u, "indices");
                     if (j_isarray(ind) && j_len(ind) == 2 && atoi(j_val(j_index(ind, 0))) == pos)
                     {
                        const char *name = j_get(u, "name");
                        if (!(*name & 0x80))
                           replace(atoi(j_val(j_index(ind, 1))) - pos, j_get(u, "name"));
                        else
                           u = NULL;
                        break;
                     }
                  }
                  if (u)
                     continue;  // found
               }
               if (entities)
               {                // URLS
                  j_t u = NULL;
                  for (u = j_first(j_find(entities, "urls")); u; u = j_next(u))
                  {
                     j_t ind = j_find(u, "indices");
                     if (j_isarray(ind) && j_len(ind) == 2 && atoi(j_val(j_index(ind, 0))) == pos)
                     {
                        replace(atoi(j_val(j_index(ind, 1))) - pos, j_get(u, "display_url"));
                        break;
                     }
                  }
                  if (u)
                     continue;  // found
               }
               if (entities)
               {                // media
                  j_t m = NULL;
                  for (m = j_first(j_find(entities, "media")); m; m = j_next(m))
                  {
                     j_t ind = j_find(m, "indices");
                     if (j_isarray(ind) && j_len(ind) == 2 && atoi(j_val(j_index(ind, 0))) == pos)
                     {
                        char *t;
                        if (asprintf(&t, "[%s]", j_get(m, "type")) < 0)
                           errx(1, "malloc");
                        replace(atoi(j_val(j_index(ind, 1))) - pos, t);
                        free(t);
                        break;
                     }
                  }
                  if (m)
                     continue;  // found
               }
               check("&amp;", "&");     // XML crap
               check("&lt;", "<");
               check("&gt;", ">");
               check("£", "GBP");      // Cannot print £
            }
            pos++;              // Count unicode pos
            if (!skip)
            {
               text++;          // Count normal text
               out[o++] = in[i];
            }
            i++;
            while ((in[i] & 0xC0) == 0x80)
            {
               if (!skip)
                  out[o++] = in[i];
               i++;
            }
            if (skip)
               skip--;          // Skipping input
         }
         out[o] = 0;
      }
      free(in);
      const char *place = j_get(j, "place.name");
      if (!text)
      {                         // No print printing if just media
         free(out);
         continue;
      }
      int more = 0;
      if (strtoull(j_get(j, "in_reply_to_user_id"), NULL, 0) == strtoull(j_get(j, "user.id"), NULL, 0))
         more = 1;
      // Simple write out
      char *msg = NULL;
      size_t l = 0;
      FILE *o = open_memstream(&msg, &l);
      fprintf(o, "+++ %s %sFROM %s%s", when, more ? "MORE " : "", at, name);
      if (place)
         fprintf(o, " IN %s", place);
      fprintf(o, " +++\n");
      if (j != t)
      {                         // Is a retweet
         const char *name = j_get(t, "user.name"),
             *at = "";
         if (*name & 0x80)
         {
            name = j_get(t, "user.screen_name");
            at = "@";
         }
         fprintf(o, "[Retweet %s%s %s]\n", at, name, j_get(t, "created_at"));
      }
      if (!more)
      {                         // Reply
         j_t reply = j_find(j, "in_reply_to_screen_name");
         if (j_isstring(reply))
            fprintf(o, "[In reply to @%s]\n", j_val(reply));
      }
      for (unsigned char *p = out; *p; p++)
         if (p[0] == 0xE2 && p[1] == 0x80 && p[2] == 0xA6)
            p[0] = p[1] = p[2] = '.';   // Ellipsis, done here as used in expanded URLs
      if (wrap)
      {                         // Line wrap
         char *p = out;
         while (*p)
         {
            int pos = 0;
            char *q = p,
                *b = p;
            while (*q && *q != '\n' && pos < 72)
            {
               if (*q == ' ')
                  p = q;
               if (*q >= ' ' && (*q & 0xC0) != 0x80)
                  pos++;
               q++;
            }
            if (!p || !*q || *q == '\n' || (q - b) < wrap / 2)
               p = q;
            fprintf(o, "%.*s\n", (int) (p - b), b);
            while (*p == ' ')
               p++;
            if (*p == '\n')
               p++;
         }
      } else
         fprintf(o, "%s\n", out);
      if (!i && footnote)
         fprintf(o, "*** %s ***\n", footnote);
      {                         // Quoted
         j_t quoted = j_find(j, "quoted_status");
         if (quoted)
         {
            const char *name = j_get(quoted, "user.name"),
                *at = "";
            if (*name & 0x80)
            {
               name = j_get(quoted, "user.screen_name");
               at = "@";
            }
            fprintf(o, "[Quoted %s%s %s]\n", at, name, j_get(quoted, "created_at"));
         }
      }
      fprintf(o, "\n\n");
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
