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

void nameuser(FILE * o, j_t u)
{                               // Write name of user (try full name is not emoji or too long, else @name)
   const char *p = j_get(u, "name"),
       *q;
   if (p)
      for (q = p; *q && !(*q & 0x80); q++);
   if (!q || *q || q > p + 20)
      fprintf(o, "@%s", j_get(u, "screen_name"));
   else
      fprintf(o, "%s", p);
}

int expand(FILE * o, j_t t)
{                               // Expand a tweet (main tweet, quoted tweet, retweeted tweet... (does not line wrap at this point)
   j_t j = j_find(t, "extended_tweet");
   if (j_isobject(j))
      t = j;                    // Use extended
   j_t entities = j_find(t, "entities");
   j_t text = j_find(t, "full_text") ? : j_find(t, "text");
   if (!j_isstring(text))
      return 0;
   const char *i = j_val(text);
   int pos = 0;                 // unicode position for replacement
   int count = 0;               // count actual text
   int skip = atoi(j_val(j_index(j_find(t, "display_text_range"), 0)) ? : "");  // Initial skipped text
   while (*i)
   {
      if (skip)
      {                         // Skip some text
         skip--;
         pos++;
         i++;
         while ((*i & 0xC0) == 0x80)
            i++;
         continue;
      }
      int match(j_t j) {
         j = j_find(j, "indices");
         if (!j_isarray(j) || j_len(j) != 2)
            return 0;
         int a = atoi(j_val(j_index(j, 0)));
         if (a != pos)
            return 0;
         int b = atoi(j_val(j_index(j, 1)));
         if (b <= a)
            return 0;
         return b - a;          // Length to skip - indicates found
      }
      for (j = j_first(j_find(entities, "urls")); !skip && j; j = j_next(j))
         if ((skip = match(j)))
            fprintf(o, "%s", j_get(j, "display_url"));
      if (skip)
         continue;
      for (j = j_first(j_find(entities, "media")); !skip && j; j = j_next(j))
         if ((skip = match(j)))
            fprintf(o, "[%s]", j_get(j, "type"));
      if (skip)
         continue;
      for (j = j_first(j_find(entities, "user_mentions")); !skip && j; j = j_next(j))
         if ((skip = match(j)))
            nameuser(o, j);
      if (skip)
         continue;
      int check(const char *from, const char *to) {     // Simple replace
         int l = strlen(from);
         if (strncmp(i, from, l))
            return 0;
         fprintf(o, "%s", to);
         while (l--)
            if ((*i++ & 0xC0) != 0x80)
               pos++;
         return 1;
      }
      if (check("&amp;", "&") || check("&lt;", "<") || check("&gt;", ">") || check("£", "GBP") || check("“", "\"") || check("”", "\"") || check("—", "--") || check("{", "[") || check("}", "]") || check("_", "-"))
         continue;
      // Next character
      count++;
      pos++;
      fputc(*i++, o);
      while ((*i & 0xC0) == 0x80)
         fputc(*i++, o);
   }
   fprintf(o, "\n");
   return count;
}

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
      long long id = strtoull(j_get(j, "user.id"), NULL, 0);
      int i = 0;
      if (idn)
      {
         for (i = 0; i < idn && id != ids[i]; i++);
         if (i == idn)
            continue;
      }

      int count = 0;
      char *msg = NULL;
      size_t msglen = 0;
      FILE *o = open_memstream(&msg, &msglen);
      int self = (strtoull(j_get(j, "in_reply_to_user_id"), NULL, 0) == strtoull(j_get(j, "user.id"), NULL, 0));
      {                         // Heading line
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
         fprintf(o, "+++ %s", when);
         if (self)
            fprintf(o, " MORE");
         fprintf(o, " FROM ");
         if (alias && idn && !i)
            fprintf(o, "%s", alias);
         else
            nameuser(o, j_find(j, "user"));
         const char *place = j_get(j, "place.name");
         if (place)
            fprintf(o, " IN %s", place);
         fprintf(o, " +++\n");
      }
      if (!self)
      {                         // reply to
         j_t reply = j_find(j, "in_reply_to_screen_name");
         if (j_isstring(reply))
            fprintf(o, "[IN REPLY TO @%s]\n", j_val(reply));
      }
      j_t retweet = j_find(j, "retweeted_status");
      if (j_isobject(retweet))
      {
         fprintf(o, "[RETWEET ");
         nameuser(o, j_find(retweet, "user"));
         fprintf(o, " %s]\n", j_get(retweet, "created_at"));
         count += expand(o, retweet);
      } else
      {
         count += expand(o, j);
         if (!i && footnote)
            fprintf(o, "*** %s ***\n", footnote);
      }
      j_t quoted = j_find(j, "quoted_status");
      if (j_isobject(quoted))
      {
         fprintf(o, "[QUOTING ");
         nameuser(o, j_find(quoted, "user"));
         fprintf(o, " %s]\n", j_get(quoted, "created_at"));
         count += expand(o, quoted);
      }
      fprintf(o, "\007\007\007\n\n");   // Gap / bell
      fclose(o);
      for (unsigned char *p = msg; *p; p++)
         if (p[0] == 0xE2 && p[1] == 0x80 && p[2] == 0xA6)
            p[0] = p[1] = p[2] = '.';   // Ellipsis, done here as used in expanded URLs
      if (wrap)
      {                         // Line wrapping
         char *was = msg,
             *p = msg;
         FILE *o = open_memstream(&msg, &msglen);
         while (*p)
         {
            int pos = 0;
            char *q = p,
                *b = p;
            while (*q && *q != '\n' && pos < wrap)
            {
               if (*q == ' ')
                  p = q;
               if ((unsigned char) *q >= ' ' && (*q & 0xC0) != 0x80)
                  pos++;
               q++;
            }
            if (!*q || *q == '\n' || (p - b) < wrap / 2)
               p = q;
            fprintf(o, "%.*s", (int) (p - b), b);
            while (*p == ' ')
               p++;
            if (*p)
               fputc('\n', o);
            if (*p == '\n')
               p++;
         }
         fclose(o);
         free(was);
      }
      if (count)
      {
         int e = mosquitto_publish(mqtt, NULL, topic, msglen, msg, 0, 0);
         if (e)
            warnx("MQTT publish failed %s (%s) %d bytes", mosquitto_strerror(e), topic, msglen);
         if (debug)
            fprintf(stderr, "%s", msg);
      }
      free(msg);
   }
   j_free(j);
   return 0;
}
