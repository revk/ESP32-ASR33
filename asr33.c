// Wrap command for ASR33 over MTT

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

int debug = 0;

int main(int argc, const char *argv[])
{
   const char *mqtthostname = NULL;
   const char *mqttusername = NULL;
   const char *mqttpassword = NULL;
   const char *mqttid = NULL;
   const char *mqttcafile = NULL;
   int mqttport = 0;
   const char *tty = NULL;
   const char *cmd = NULL;
   char **cmdargs = NULL;
   int cmdargc = 0;
   int outonly = 0;
   {                            // POPT
      poptContext optCon;       // context for parsing command-line options
      const struct poptOption optionsTable[] = {
         { "mqtt-hostname", 'h', POPT_ARG_STRING, &mqtthostname, 0, "MQTT hostname", "hostname" },
         { "mqtt-username", 'u', POPT_ARG_STRING, &mqttusername, 0, "MQTT username", "username" },
         { "mqtt-password", 'p', POPT_ARG_STRING, &mqttpassword, 0, "MQTT password", "password" },
         { "mqtt-ca", 'C', POPT_ARG_STRING, &mqttcafile, 0, "MQTT CA", "filename" },
         { "mqtt-port", 0, POPT_ARG_INT, &mqttport, 0, "MQTT port", "port" },
         { "mqtt-id", 0, POPT_ARG_STRING, &mqttid, 0, "MQTT id", "id" },
         { "tty", 0, POPT_ARG_STRING, &tty, 0, "ASR33", "id" },
         { "out-only", 'O', POPT_ARG_NONE, &outonly, 0, "Out only so no need to leave on" },
         { "debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug" },
         POPT_AUTOHELP { }
      };

      optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp(optCon, "command [args]");

      int c;
      if ((c = poptGetNextOpt(optCon)) < -1)
         errx(1, "%s: %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(c));

      if (!poptPeekArg(optCon))
      {
         poptPrintUsage(optCon, stderr, 0);
         errx(1, "Command required");
      }

      cmd = poptGetArg(optCon);
      cmdargs = realloc(cmdargs, sizeof(*cmdargs) * (++cmdargc));
      cmdargs[cmdargc - 1] = (void *) cmd;

      while (poptPeekArg(optCon))
      {
         cmdargs = realloc(cmdargs, sizeof(*cmdargs) * (++cmdargc));
         cmdargs[cmdargc - 1] = (void *) poptGetArg(optCon);
      }
      cmdargs = realloc(cmdargs, sizeof(*cmdargs) * (++cmdargc));
      cmdargs[cmdargc - 1] = NULL;
      poptFreeContext(optCon);
   }
   pid_t pid = 0;
   int pts = -1;
   int busy = 0;
   int esc = 0;

   int e = mosquitto_lib_init();
   if (e)
      errx(1, "MQTT init failed %s", mosquitto_strerror(e));
   void dofork(void) {
      if (pid)
         return;
      pts = posix_openpt(O_RDWR);
      if (pts < 0)
         err(1, "PTS");
      grantpt(pts);
      unlockpt(pts);
      pid = fork();
      if (pid < 0)
         err(1, "fork");
      if (!pid)
      {
         int f = open(ptsname(pts), O_RDWR);
         if (f < 0)
            err(1, "PTS");
         dup2(f, fileno(stdin));
         dup2(f, fileno(stdout));
         dup2(f, fileno(stderr));
         close(f);
         close(pts);
         setenv("TERM", "tty33", 1);
         if (execvp(cmd, cmdargs) < 0)
            err(1, "exec of %s failed", cmd);
      }
      if (debug)
         warnx("Running %s", cmd);
   }
   void donefork(void) {
      if (!pid)
         return;
      kill(pid, SIGTERM);
      close(pts);
      pts = -1;
      pid = 0;
   }
   void connect(struct mosquitto *mqtt, void *obj, int rc) {
      char *sub;
      if (asprintf(&sub, "event/ASR33/%s/rx", tty ? : "+") < 0)
         errx(1, "malloc");
      int e = mosquitto_subscribe(mqtt, NULL, sub, 0);
      if (e)
         warnx("MQTT subscribe failed %s (%s)", mosquitto_strerror(e), sub);
      if (debug)
         warnx("sub %s", sub);
      free(sub);
      if (asprintf(&sub, "state/ASR33/%s/busy", tty ? : "+") < 0)
         errx(1, "malloc");
      e = mosquitto_subscribe(mqtt, NULL, sub, 0);
      if (e)
         warnx("MQTT subscribe failed %s (%s)", mosquitto_strerror(e), sub);
      if (debug)
         warnx("sub %s", sub);
      free(sub);
      if (debug)
         warnx("Connect");
      dofork();

   }
   void disconnect(struct mosquitto *mqtt, void *obj, int rc) {
      if (debug)
         warnx("Disconnect");
      donefork();
   }
   void message(struct mosquitto *mqtt, void *obj, const struct mosquitto_message *msg) {
      if (debug)
         warnx("msg %s", msg->topic);
      int l = strlen(msg->topic);
      if (l >= 3 && !strcmp(msg->topic + l - 3, "/rx"))
      {
         for (int i = 0; i < msg->payloadlen; i++)
         {
            // ESC prefix on a letter causes it to (stay) upper case.
            char c = (((char *) msg->payload)[i] &= 0x7F);      // Parity
            if (esc)
            {
               esc = 0;
            } else if (c == 27)
            {
               esc = 1;
               c = 0;
            } else if (isupper(c))
               c = tolower(c);
            if (c)
               write(pts, &c, 1);
         }
         if (debug)
            warnx("tx %.*s", msg->payloadlen, msg->payload);
         return;
      }
      if (l >= 5 && !strcmp(msg->topic + l - 5, "/busy"))
      {
         if (*(char *) msg->payload == '1')
            busy = 1;
         else
            busy = 0;
         if (debug)
            warnx("Busy %d", busy);
         return;
      }
   }

   struct mosquitto *mqtt = mosquitto_new(mqttid, 1, NULL);
   if (mqttusername)
   {
      e = mosquitto_username_pw_set(mqtt, mqttusername, mqttpassword);
      if (e)
         errx(1, "MQTT auth failed %s", mosquitto_strerror(e));
   }
   if (mqttcafile && (e = mosquitto_tls_set(mqtt, mqttcafile, NULL, NULL, NULL, NULL)))
      warnx("MQTT cert failed (%s) %s", mqttcafile, mosquitto_strerror(e));
   mosquitto_connect_callback_set(mqtt, connect);
   mosquitto_disconnect_callback_set(mqtt, disconnect);
   mosquitto_message_callback_set(mqtt, message);
   e = mosquitto_connect(mqtt, mqtthostname ? : "localhost", mqttport ? : mqttcafile ? 8883 : 1883, 60);
   if (e)
      errx(1, "MQTT connect failed %s", mosquitto_strerror(e));
   mosquitto_loop_start(mqtt);

   while (1)
   {
      if (debug)
         warnx("idle");
      if (!pid)
         sleep(1);
      else
      {
         if (debug)
            warnx("rx start");
         char buf[1024];
         ssize_t l;
         char *topic = NULL;
         if (!outonly)
         {
            // Manual on
            if (asprintf(&topic, "command/ASR33/%s/on", tty ? : "*") < 0)
               errx(1, "malloc");
            int e = mosquitto_publish(mqtt, NULL, topic, 0, "", 0, 0);
            if (e)
               warnx("MQTT publish failed %s (%s)", mosquitto_strerror(e), topic);
            free(topic);
            // No local echo
            if (asprintf(&topic, "command/ASR33/%s/noecho", tty ? : "*") < 0)
               errx(1, "malloc");
            e = mosquitto_publish(mqtt, NULL, topic, 0, "", 0, 0);
            if (e)
               warnx("MQTT publish failed %s (%s)", mosquitto_strerror(e), topic);
            free(topic);
         }
         // Topic for the text
         if (asprintf(&topic, "command/ASR33/%s/text", tty ? : "*") < 0)
            errx(1, "malloc");
         while ((l = read(pts, buf, sizeof(buf))) > 0)
         {
            if (debug)
               warnx("rx (%d) %.*s", (int) l, (int) l, buf);
            int e = mosquitto_publish(mqtt, NULL, topic, l, buf, 0, 0);
            if (e)
               warnx("MQTT publish failed %s (%s) %d bytes", mosquitto_strerror(e), topic, l);
            do
               usleep(100000);
            while (busy);
         }
         free(topic);
         donefork();
         // dofork(); // restart
         break;                 // closed
      }
   }
   // No local echo
   if (asprintf(&topic, "command/ASR33/%s/noecho", tty ? : "*") < 0)
      errx(1, "malloc");
   e = mosquitto_publish(mqtt, NULL, topic, 0, "", 0, 0);
   if (e)
      warnx("MQTT publish failed %s (%s)", mosquitto_strerror(e), topic);
   free(topic);
   sleep(1);
   mosquitto_lib_cleanup();
   return 0;
}
