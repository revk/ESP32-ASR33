// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0

#include "revk.h"
#include <driver/gpio.h>
#include "softuart.h"
#include "tty.h"
#include "adventesp.h"

#define	NUL	0
#define	EOT	4
#define	WRU	5
#define	RU	6
#define BEL	7
#define	LF	10
#define	CR	13
#define	DC1	0x11
#define	DC2	0x12
#define	DC3	0x13
#define	DC4	0x14
#define	RO	0x7F

#define	MAXRX	256             // Line max

struct
{
   uint8_t on:1;                // Power is on
   uint8_t brk:1;               // Break condition
   uint8_t busy:1;              // Busy state (tx queue is low)
   uint8_t pressed:1;           // Button pressed
   uint8_t xoff:1;              // Manual no echo
   uint8_t doecho:1;            // Do echo (set each start up)
   uint8_t dobig:1;             // Do big lettering on tape
   uint8_t docave:1;            // Run advent()
   uint8_t suppress:1;          // Suppress WRU
} b = { 0 };

volatile int8_t power = 0;      // power request, -1 means want off, 1 means want on, 2 means want on with long timeout
uint8_t pos = 0;                // Pos for wrapping
int lsock = -1;                 // Listen socket
int csock = -1;                 // Connected sockets
char line[MAXRX + 1];           // Rx line buffer
int64_t lastrx = 0;             // Last rx
int64_t done = 0;               // When to turn off
uint32_t rxp = 0;               // Rx line buffer pointer
uint8_t hayes = 0;              // Hayes +++ counter

volatile uint8_t rxws[64];      // rx for ws
volatile uint8_t rxwsp = 0;     // tx for ws
static SemaphoreHandle_t rxws_mutex = NULL;

static httpd_handle_t webserver = NULL;

const unsigned char small_f[256][5] = {
#include "smallfont.h"
};

void
sendbyte (uint8_t b)
{
   tty_tx (b);
   b &= 0x7F;
   if (b == CR)
      pos = 0;
   else if (b >= ' ' && b < RO)
      pos++;
}

void
cr (void)
{                               // Do a carriage return
   sendbyte (pe (CR));
}

void
nl (void)
{                               // Do new line
   cr ();
   sendbyte (pe (LF));
}

void
sendstring (const char *c)
{                               // Simple text string, with wrap and \n for line breaks
   if (c)
      while (*c)
      {
         if (*c == LF)
            nl ();
         else
         {
            if (pos >= linelen)
               nl ();
            sendbyte (pe (*c));
         }
         c++;
      }
}

void
sendutf8 (int len, const char *value)
{
   while (len--)
   {
      uint32_t b = *value++;
      // We assume this is utf-8
      if ((b & 0xC0) == 0x80)
         b = 0x7F;              // Silly, not even utf-8, rub out
      else if ((b & 0xF8) == 0xF0 && len >= 3 && (value[0] & 0xC0) == 0x80 && (value[1] & 0xC0) == 0x80
               && (value[2] & 0xC0) == 0x80)
      {
         b = (value[2] & 0x3F) + ((value[1] & 0x3F) << 6) + ((value[0] & 0x3F) << 12) + ((b & 0x07) << 18);
         value += 3;
         len -= 3;
      } else if ((b & 0xF0) == 0xE0 && len >= 2 && (value[0] & 0xC0) == 0x80 && (value[1] & 0xC0) == 0x80)
      {
         b = (value[1] & 0x3F) + ((value[0] & 0x3F) << 6) + ((b & 0x0F) << 12);
         value += 2;
         len -= 2;
      } else if ((b & 0xE0) == 0xC0 && len >= 1 && (value[0] & 0xC0) == 0x80)
      {
         b = (value[0] & 0x3F) + ((b & 0x1F) << 6);
         value += 1;
         len -= 1;
      }
      // Handle some unicode we understand
      if (b >= 0x1D670 && b < 0x1D670 + 26)
         b = 'A' + b - 0x1D670; // Maths monospace characters
      else if (b >= 0x1D68A && b < 0x1D68A + 26)
         b = 'a' + b - 0x1D68A;
      else if (b >= 0x1D7F6 && b < 0x1D7F6 + 10)
         b = '0' + b - 0x1D7F6;
      else if (b == 0x2014)
         b = '-';               // Common hyphen (long)
      else if (b == 0x2018 || b == 0x2019)
         b = '\'';              // Common quote escape
      else if (b == 0x201c || b == 0x201d)
         b = '"';               // Common quote escape
      if (b >= 0x80)
         b = 0x5E;              // Other unicode so print as Up arrow
      if (b >= ' ' && b < 0x7F && pos >= linelen)
         nl ();                 // Force newline as would overprint
      if (b == LF)
         nl ();                 // We want a new line (does CR too)
      else if (b == CR)
         cr ();                 // We want a carriage return
      else
         sendbyte (pe (b));
   }
}

int
queuebig (int c)
{                               // Send a big character for tape
   if (c >= sizeof (small_f) / sizeof (&small_f))
      return 0;
   const unsigned char *d = small_f[c];
   if (!*d && c >= 'a' && c <= 'z')
      d = small_f[c - 32];      // Try upper case
   int l = 5;
   while (l && !d[l - 1])
      l--;
   if (c == ' ' && l < 3)
      l = 3;
   if (!l)
      return 0;
   while (l--)
   {
      uint8_t c = *d++;
      sendbyte (c);
      c &= 0x7f;
      if (!nodc4 && c == DC4)
         sendbyte (DC2);
      if (c == WRU)
      {
         b.suppress = 1;
         lastrx = 0;
      }
   }
   return 1;
}

void
reportstate (void)
{
   uint64_t t = esp_timer_get_time ();
   jo_t j = jo_object_alloc ();
   jo_litf (j, "up", "%d.%06d", (uint32_t) (t / 1000000LL), (uint32_t) (t % 1000000LL));
   jo_bool (j, "power", b.on);
   jo_bool (j, "brk", b.brk);
   jo_bool (j, "busy", b.busy);
   if (port)
      jo_bool (j, "connected", csock >= 0);
   revk_state (NULL, &j);
}

void
power_off (void)
{
   if (!b.on)
      return;                   // Already off
   power = 0;
   rxp = 0;
   hayes = 0;
   b.doecho = !noecho;
   b.xoff = 0;
   reportstate ();
   tty_flush ();
   if (pwr.set)
      tty_xoff ();
   usleep (100000);
   if (mtr.set || *mtrtopic)
   {                            // Motor direct control, off
      if (*mtrtopic)
         revk_mqtt_send_raw (mtrtopic, 0, "0", 0);
      revk_gpio_set (mtr, 0);   // Off
      usleep (1000 * timemtroff);
   }
   if (pwr.set || *pwrtopic)
   {
      if (*pwrtopic)
         revk_mqtt_send_raw (pwrtopic, 0, "0", 0);
      revk_gpio_set (pwr, 0);   // Off
      usleep (1000 * timepwroff);
   }
   b.on = 0;
   reportstate ();
}

void
power_on (void)
{
   if (!b.on)
   {
      if (pwr.set || *pwrtopic)
      {                         // Power, direct control, on
         if (*pwrtopic)
            revk_mqtt_send_raw (pwrtopic, 0, "1", 0);
         revk_gpio_set (pwr, 1);        // On
         usleep (1000 * timepwron);     // Min is 20ms for zero crossing, and 9ms for one bit for solenoid, but can be longer for safety
      }
      if (mtr.set || *mtrtopic)
      {                         // Motor, direct control, on
         if (*mtrtopic)
            revk_mqtt_send_raw (mtrtopic, 0, "1", 0);
         revk_gpio_set (mtr, 1);        // On
         usleep (1000 * timemtron);     // Min 100ms for a null character from power off, and some for motor to start and get to speed
      }
      b.on = 1;
      reportstate ();
      if (pwr.set)
         tty_xon ();
   }
   done = esp_timer_get_time () + 1000 * (power > 1 ? timekeyidle : timeremidle);
}

jo_t
jo_stats (char clear)
{
   softuart_stats_t s;
   tty_stats (&s, clear);
   jo_t j = jo_object_alloc ();
   jo_int (j, "tx", s.tx);
   jo_int (j, "rx", s.rx);
   jo_int (j, "rxbadstart", s.rxbadstart);
   jo_int (j, "rxbadstop", s.rxbadstop);
   jo_int (j, "rxbad0", s.rxbad0);
   jo_int (j, "rxbadish0", s.rxbadish0);
   jo_int (j, "rxbad1", s.rxbad1);
   jo_int (j, "rxbadish1", s.rxbadish1);
   jo_int (j, "rxbadp", s.rxbadp);
   jo_bool (j, "rxlevel", revk_gpio_get (rx));
   return j;
}

const char *
app_callback (int client, const char *prefix, const char *target, const char *suffix, jo_t j)
{
   if (client || target || !prefix || strcmp (prefix, "command"))
      return NULL;              // Not what we want
   if (!strcmp (suffix, "status"))
      reportstate ();
   if (!strcmp (suffix, "connect"))
   {
      if (*pwrtopic)
         revk_mqtt_send_raw (pwrtopic, 0, b.on ? "1" : "0", 0);
      if (*mtrtopic)
         revk_mqtt_send_raw (mtrtopic, 0, b.on ? "1" : "0", 0);
      reportstate ();
   }
   if (!strcmp (suffix, "restart"))
      power = -1;
   if (!strcmp (suffix, "cave"))
      b.docave = 1;
   if (!strcmp (suffix, "on"))
      power = 2;
   if (!strcmp (suffix, "off"))
      power = -1;
   if (!strcmp (suffix, "echo"))
      b.doecho = 1;
   if (!strcmp (suffix, "noecho"))
      b.doecho = 0;
   if (!strcmp (suffix, "break"))
   {
      power = 1;
      int chars = 10;
      if (j && jo_here (j) == JO_NUMBER)
         chars = jo_read_int (j);
      tty_break (chars);
   }
   if (!strcmp (suffix, "uartstats"))
   {
      jo_t j = jo_stats (1);
      revk_info ("uartstats", &j);
   }

   if (!strcmp (suffix, "tape") || !strcmp (suffix, "taperaw") || !strcmp (suffix, "text") || !strcmp (suffix, "line")
       || !strcmp (suffix, "bell"))
   {                            // Plain text functions - simple JSON string
      int len = jo_strlen (j);
      if (len < 0)
         return "JSON string expected";
      char *buf = jo_strdup (j),
         *value = buf;
      if (!buf)
         return "Malloc";
      if (!strcmp (suffix, "tape") || !strcmp (suffix, "taperaw"))
      {                         // Punched tape text
         power = 1;
         if (!suffix[4])
         {
            if (!nodc4)
               sendbyte (DC2);  // Tape on
            for (int i = 0; i < tapelead; i++)
               sendbyte (NUL);
         }
         if (!strcmp (suffix, "taperaw"))
            while (len--)
               sendbyte (*value++);     // Raw
         else                   // Large text
            while (len--)
            {
               if (!queuebig (*value++))
                  continue;
               if (len)
                  sendbyte (NUL);
            }
         if (!suffix[4])
         {
            for (int i = 0; i < tapetail; i++)
               sendbyte (NUL);
            if (!nodc4)
               sendbyte (DC4);  // Tape off
         }
      }
      if (!strcmp (suffix, "text") || !strcmp (suffix, "line") || !strcmp (suffix, "bell"))
      {
         power = 1;
         sendutf8 (len, value);
         if (!strcmp (suffix, "line") || !strcmp (suffix, "bell"))
         {
            cr ();
            nl ();
            if (!strcmp (suffix, "bell"))
               sendbyte (pe (BEL));
         }
      }
      return "";
   }
   if (!strcmp (suffix, "tx") || !strcmp (suffix, "txraw") || !strcmp (suffix, "raw") || !strcmp (suffix, "punch")
       || !strcmp (suffix, "punchraw"))
   {
      // Functions that expect hex data
      int len = jo_strncpy (j, NULL, 0);
      if (len < 0)
         return "Expecting JSON string";

      char *buf = malloc (len),
         *value = buf;
      if (!buf)
         return "Malloc";
      jo_strncpy (j, buf, len);
      if (!strcmp (suffix, "tx") || !strcmp (suffix, "txraw") || !strcmp (suffix, "raw"))
      {                         // raw send
         power = 1;
         while (len--)
            sendbyte (*value++);
      }
      if (!strcmp (suffix, "punch") || !strcmp (suffix, "punchraw"))
      {                         // Raw punched data (with DC2/DC4)
         power = 1;
         if (!nodc4)
            sendbyte (DC2);     // Tape on
         if (!suffix[5])
         {
            for (int i = 0; i < tapelead; i++)
               sendbyte (NUL);
         }
         while (len--)
         {
            uint8_t c = *value++;
            sendbyte (c);
            c &= 0x7f;
            if (!nodc4 && c == DC4)
               sendbyte (DC2);  // Turn tape back on
            if (c == WRU)
            {
               b.suppress = 1;  // Suppress WRU response
               lastrx = 0;
            }
         }
         if (!suffix[5])
         {
            for (int i = 0; i < tapetail; i++)
               sendbyte (NUL);
         }
         if (!nodc4)
         {
            sendbyte (DC4);     // Tape off
            nl ();              // Tidy
         }
      }
      free (buf);
   }
   return "";
}

void
asr33_main (void *param)
{
   b.doecho = !noecho;

   tty_setup ();

   revk_gpio_input (run);
   revk_gpio_output (pwr, 0);
   revk_gpio_output (mtr, 0);
   if (port)
      lsock = socket (AF_INET6, SOCK_STREAM, 0);        // IPPROTO_IPV6);
   if (lsock >= 0)
   {
      struct sockaddr_storage dest_addr;
      struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *) &dest_addr;
      bzero (&dest_addr_ip6->sin6_addr.un, sizeof (dest_addr_ip6->sin6_addr.un));
      dest_addr_ip6->sin6_family = AF_INET6;
      dest_addr_ip6->sin6_port = htons (port);
      if (bind (lsock, (struct sockaddr *) &dest_addr, sizeof (dest_addr)))
      {
         close (lsock);
         lsock = -1;
      }
   }
   if (lsock >= 0 && listen (lsock, 1))
   {
      close (lsock);
      lsock = -1;
   }
   if (pwr.set)
      tty_xoff ();
   else
      tty_xon ();
   void doconnect (const char *line)
   {
      if (csock >= 0)
         return;
      const struct addrinfo hints = {
         .ai_family = AF_UNSPEC,
         .ai_socktype = SOCK_STREAM,
         .ai_flags = AI_CANONNAME,
      };
      char ports[20];
      sprintf (ports, "%d", port);
      struct addrinfo *res = NULL,
         *a;
      int err = getaddrinfo (line, ports, &hints, &res);
      if (err || !res)
         sendstring ("+++ HOST NAME NOT FOUND +++\n");
      else
      {
         for (a = res; a; a = a->ai_next)
         {
            int s = socket (a->ai_family, a->ai_socktype, a->ai_protocol);
            if (s >= 0)
            {
               if (!connect (s, a->ai_addr, a->ai_addrlen))
               {
                  csock = s;
                  jo_t j = jo_object_alloc ();
                  if (res && res->ai_canonname)
                     jo_string (j, "target", res->ai_canonname);
                  revk_event ("connect", &j);
                  break;
               }
               close (s);
            }
         }
         freeaddrinfo (res);
      }
   }
   if (revk_gpio_get (run))
      b.pressed = 1;            // Initial state for RUN/STOP button
   void dorun (void)
   {                            // RUN button manual start
      power = 2;
      if (*autoconnect)
         doconnect (autoconnect);
      if (csock < 0)
      {
         if (autoprompt)
            hayes = 3;
         else if (autocave)
            b.docave = 1;
      }
   }
   if (autoon)
      dorun ();
   while (1)
   {
      usleep (10000);
      int64_t now = esp_timer_get_time ();
      int64_t gap = now - lastrx;
      if (csock >= 0)
         revk_blink (1, 0, tty_tx_waiting ()? "CR" : "C");
      else if (hayes > 3)
         revk_blink (1, 0, "M");
      else if (b.on)
         revk_blink (1, 0, tty_tx_waiting ()? "RG" : "G");
      else
         revk_blink (0, 0, NULL);
      // Handle RUN button
      if (revk_gpio_get (run))
      {                         // RUN button
         if (!b.pressed)
         {                      // Button pressed
            b.pressed = 1;
            if (b.on)
               power = -1;      // Turn off
            else
               dorun ();
         }
      } else
         b.pressed = 0;
      // Handle power change
      if (power < 0)
      {                         // Power off
         if (b.on && !tty_tx_waiting ())
         {                      // Do power off once tx done
            if (csock >= 0)
            {                   // Close connection
               close (csock);
               csock = -1;
               jo_t j = jo_object_alloc ();
               jo_string (j, "reason", "power");
               revk_event ("closed", &j);
            }
            power_off ();
         }
      } else if (power)
      {
         if (!b.on)
            power_on ();
      }
      // Check tx buffer usage
      if (tty_tx_space () < 4096)
      {
         if (!b.busy)
         {
            b.busy = 1;
            reportstate ();
         }
      } else
      {
         if (b.busy)
         {
            b.busy = 0;
            reportstate ();
         }
      }
      // Handle incoming connection
      if (lsock >= 0 && csock < 0)
      {                         // Allow for connection
         fd_set s;
         FD_ZERO (&s);
         FD_SET (lsock, &s);
         struct timeval timeout = { };
         if (select (lsock + 1, &s, NULL, NULL, &timeout) > 0)
         {
            struct sockaddr_storage source_addr;        // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof (source_addr);
            csock = accept (lsock, (struct sockaddr *) &source_addr, &addr_len);
            char addr_str[40] = "";
            if (source_addr.ss_family == PF_INET)
               inet_ntoa_r (((struct sockaddr_in *) &source_addr)->sin_addr, addr_str, sizeof (addr_str) - 1);
            else if (source_addr.ss_family == PF_INET6)
               inet6_ntoa_r (((struct sockaddr_in6 *) &source_addr)->sin6_addr, addr_str, sizeof (addr_str) - 1);
            jo_t j = jo_object_alloc ();
            jo_string (j, "ip", addr_str);
            revk_event ("connect", &j);
            power = 1;
         }
      }
      if (hayes == 3 && gap > 1000000)
      {                         // End of Hayes +++ escape sequence
         hayes++;               // Command prompt
         rxp = 0;
         sendstring ("\nASR33 CONTROLLER (BUILD ");
         sendstring (revk_version);
         sendstring (") ASR33.REVK.UK\n");
         if (revk_link_down ())
         {
            sendstring ("OFFLINE (SSID ");
            sendstring (revk_wifi ());
            sendstring (")\n");
         } else if (port)
         {                      // Online
            sendstring ("LISTENING ON ");
            char temp[50];
#ifdef CONFIG_LWIP_IPV6
            esp_ip6_addr_t ip;
            if (!esp_netif_get_ip6_global (sta_netif, &ip))
            {
               sprintf (temp, IPV6STR, IPV62STR (ip));
               sendstring (temp);
               sendstring (" & ");
            }
#endif
            esp_netif_ip_info_t ip4;
            if (!esp_netif_get_ip_info (sta_netif, &ip4))
            {
               inet_ntoa_r (ip4.ip, temp, sizeof (temp) - 1);
               sendstring (temp);
            }
            sendstring ("\nENTER IP/DOMAIN TO MAKE CONNECTION");
            sprintf (temp, " (USING TCP PORT %d)", port);
            sendstring (temp);
            sendstring ("\n");
            if (!nocave)
               sendstring ("OR ");
         }
         if (!nocave)
            sendstring ("WOULD YOU LIKE TO PLAY A GAME? (Y/N)\n> ");
      }
      // Check tcp
      if (csock >= 0)
      {
         fd_set s;
         FD_ZERO (&s);
         FD_SET (csock, &s);
         struct timeval timeout = { };
         if (select (csock + 1, &s, NULL, NULL, &timeout) > 0)
         {
            uint8_t b = 0;
            int len = recv (csock, &b, 1, 0);
            if (len <= 0)
            {                   // Closed
               close (csock);
               csock = -1;
               jo_t j = jo_object_alloc ();
               jo_string (j, "reason", "close");
               revk_event ("closed", &j);
            } else
               sendbyte (b);    // Raw send to teletype
         }
      }
      int len = tty_rx_ready ();
      if (!len)
      {                         // Nothing waiting and not break
         if (b.brk)
         {                      // End of break
            b.brk = 0;
            reportstate ();
         }
      } else if (len < 0)
      {                         // Break
         if (!b.brk)
         {                      // Start of break
            b.brk = 1;
            hayes = 0;
            rxp = 0;
            reportstate ();
            if (csock >= 0)
            {                   // Close connection
               close (csock);
               csock = -1;
               jo_t j = jo_object_alloc ();
               jo_string (j, "reason", "break");
               revk_event ("closed", &j);
               power = -1;
            }
         }
      } else if (len > 0)
      {
         if (power < 0)
            power = 0;          // Abort power off
         if (!b.on)
            dorun ();           // Must be not using power controls, so turn on for rx data
         uint8_t byte = tty_rx ();
         xSemaphoreTake (rxws_mutex, portMAX_DELAY);
         if (rxwsp < sizeof (rxws))
            rxws[rxwsp++] = byte;
         xSemaphoreGive (rxws_mutex);
         if (csock >= 0)
            send (csock, &byte, 1, 0);  // Connected via TCP
         else
         {                      // Not connected via TCP
            if (gap > 250000)
               b.suppress = 0;  // Suppressed WRU response timeout can end
            if (hayes > 3)
            {                   // Hayes command prompt only
               if (byte == pe (CR) || byte == pe (LF))
               {                // Command
                  sendstring ("\n");
                  line[rxp] = 0;
                  hayes = 0;
                  rxp = 0;
                  if (!strcmp (line, "Y") || !strcmp (line, "YES"))
                     b.docave = 1;
                  else if (!strcmp (line, "N") || !strcmp (line, "NO"))
                  {
                     sendstring ("SHAME, BYE\n");
                     cheese ();
                  } else if (*line)
                  {             // DNS/IP?
                     doconnect (line);
                     sendstring (csock < 0 ? "+++ COULD NOT CONNECT +++\r\n" : "+++ CONNECTED +++\r\n");
                     if (csock < 0)
                        cheese ();
                  } else
                  {             // Blank input
                     sendstring ("OK, BYE\r\n");
                     cheese ();
                  }
               } else if ((byte & 0x7f) >= ' ' && rxp < MAXRX)
               {
                  sendbyte (byte);      // echo
                  line[rxp++] = (byte & 0x7F);
               }
            } else
            {
               if (!hayes)
               {
                  if (byte == pe ('+') && !rxp && gap > 1000000)
                     hayes++;
               } else
               {
                  if (byte != pe ('+') || hayes >= 3 || gap > 1000000)
                     hayes = 0;
                  else
                     hayes++;
               }
               if (!b.suppress)
               {
                  jo_t j = jo_object_alloc ();
                  jo_int (j, "byte", byte);
                  revk_event ("rx", &j);
                  if (byte == pe (EOT))
                     power = -1;        // EOT to shut down
                  if ((byte & 0x7F) == LF || (byte & 0x7F) == CR)
                  {
                     jo_t j = jo_create_alloc ();
                     jo_stringn (j, NULL, (void *) line, rxp);
                     revk_event ("line", &j);
                     rxp = 0;
                  } else if ((byte & 0x7f) >= ' ' && rxp < MAXRX)
                     line[rxp++] = (byte & 0x7F);
                  if (b.doecho)
                  {             // Handling local characters and echoing (maybe, depends on xoff too)
                     if (b.dobig)
                     {          // Doing large lettering
                        if (byte == pe (DC4))
                        {       // End
                           for (int i = 0; i < 9; i++)
                              sendbyte (NUL);
                           sendbyte (pe (DC4));
                           b.dobig = 0;
                        } else if (byte == pe (byte) && (byte & 0x7F) >= 0x20 && queuebig (byte & 0x7F))
                           sendbyte (NUL);
                     } else if (byte == pe (DC2) && b.doecho && !nobig)
                     {          // Start big lettering
                        sendbyte (pe (DC2));
                        for (int i = 0; i < 10; i++)
                           sendbyte (NUL);
                        b.dobig = 1;
                     }
                     // else if (byte == pe(RU) && !nocave) b.docave = 1;
                     else if (byte == pe (DC1))
                        b.xoff = 0;
                     else if (byte == pe (DC3))
                        b.xoff = 1;
                     else if (byte == pe (WRU) && (!nover || *wru))
                     {          // WRU
                        // See 3.27 of ISS 8, SECTION 574-122-700TC
                        sendbyte (pe (CR));     // CR
                        sendbyte (pe (LF));     // LF
                        sendbyte (pe (0x7F));   // RO
                        sendstring (wru);
                        if (!nover)
                        {
                           if (*wru)
                              sendbyte (pe (' '));
                           sendstring (revk_app);
                           sendstring (" BUILD ");
                           sendstring (revk_version);
                        }
                        sendbyte (pe (CR));     // CR
                        sendbyte (pe (LF));     // LF
                        if (ack)
                           sendbyte (pe (ack)); // ACK
                     } else if (!b.xoff)
                        sendbyte (byte);
                  }
               }
            }
         }
         lastrx = now;
      }
      if (!tty_tx_waiting () && csock < 0)
      {                         // Nothing to send
         if (b.docave)
         {                      // Let's play a game
            b.docave = 0;
            extern int advent (void);
            if (!b.busy)
            {
               b.busy = 1;
               reportstate ();
            }
            revk_blink (1, 0, "B");
            advent ();
            power = -1;
         } else if (b.on && now > done)
            power = -1;
      } else
         done = now + 1000 * (power > 1 ? timekeyidle : timeremidle);
   }
}


static void
register_uri (const httpd_uri_t * uri_struct)
{
   esp_err_t res = httpd_register_uri_handler (webserver, uri_struct);
   if (res != ESP_OK)
   {
      ESP_LOGE (TAG, "Failed to register %s, error code %d", uri_struct->uri, res);
   }
}

static void
register_get_uri (const char *uri, esp_err_t (*handler) (httpd_req_t * r))
{
   httpd_uri_t uri_struct = {
      .uri = uri,
      .method = HTTP_GET,
      .handler = handler,
   };
   register_uri (&uri_struct);
}

static void
register_ws_uri (const char *uri, esp_err_t (*handler) (httpd_req_t * r))
{
   httpd_uri_t uri_struct = {
      .uri = uri,
      .method = HTTP_GET,
      .handler = handler,
      .is_websocket = true,
   };
   register_uri (&uri_struct);
}

static esp_err_t
web_live (httpd_req_t * req)
{                               // Web socket
   int fd = httpd_req_to_sockfd (req);
   void wsend (jo_t * jp)
   {
      char *js = jo_finisha (jp);
      if (js)
      {
         httpd_ws_frame_t ws_pkt;
         memset (&ws_pkt, 0, sizeof (httpd_ws_frame_t));
         ws_pkt.payload = (uint8_t *) js;
         ws_pkt.len = strlen (js);
         ws_pkt.type = HTTPD_WS_TYPE_TEXT;
         httpd_ws_send_frame_async (req->handle, fd, &ws_pkt);
         free (js);
      }
   }
   esp_err_t status (void)
   {
      jo_t j = jo_stats (0);
      const char *reason;
      int t = revk_shutting_down (&reason);
      if (t)
         jo_string (j, "shutdown", reason);
      jo_bool (j, "power", b.on);
      jo_bool (j, "brk", b.brk);
      jo_bool (j, "busy", b.busy);
      xSemaphoreTake (rxws_mutex, portMAX_DELAY);
      if (rxwsp)
      {
         for (uint8_t i = 0; i < rxwsp; i++)
            rxws[i] &= 0x7F;
         jo_stringn (j, "data", (void *) rxws, rxwsp);
         rxwsp = 0;
      }
      xSemaphoreGive (rxws_mutex);
      wsend (&j);
      return ESP_OK;
   }
   if (req->method == HTTP_GET)
      return status ();         // Send status on initial connect
   // received packet
   httpd_ws_frame_t ws_pkt;
   uint8_t *buf = NULL;
   memset (&ws_pkt, 0, sizeof (httpd_ws_frame_t));
   ws_pkt.type = HTTPD_WS_TYPE_TEXT;
   esp_err_t ret = httpd_ws_recv_frame (req, &ws_pkt, 0);
   if (ret)
      return ret;
   if (!ws_pkt.len)
      return status ();         // Empty string
   buf = calloc (1, ws_pkt.len + 1);
   if (!buf)
      return ESP_ERR_NO_MEM;
   ws_pkt.payload = buf;
   ret = httpd_ws_recv_frame (req, &ws_pkt, ws_pkt.len);
   if (!ret)
   {
      jo_t j = jo_parse_mem (buf, ws_pkt.len);
      if (j)
      {                         // handle commands
         if (jo_find (j, "text"))
         {
            int len = jo_strlen (j);
            if (len > 0)
            {
               power = 1;
               char *text = jo_strdup (j);
               sendutf8 (len, text);
               nl ();
               free (text);
            }
         }
         if (jo_find (j, "tape"))
         {
            int len = jo_strlen (j);
            if (len > 0)
            {
               power = 1;
               if (!nodc4)
                  sendbyte (DC2);       // Tape on
               for (int i = 0; i < tapelead; i++)
                  sendbyte (NUL);
               char *text = jo_strdup (j),
                  *value = text;
               while (len--)
               {
                  if (!queuebig (*value++))
                     continue;
                  if (len)
                     sendbyte (NUL);
               }
               free (text);
               for (int i = 0; i < tapetail; i++)
                  sendbyte (NUL);
               if (!nodc4)
               {
                  sendbyte (DC4);       // Tape off
                  nl ();        // Tidy
               }
            }
         }
         if (jo_find (j, "wru"))
         {
            power = 1;
            sendbyte (pe (WRU));
         }
         if (jo_find (j, "clear"))
         {
            tty_stats (NULL, 1);
         }
         jo_free (&j);
      }
   }
   free (buf);
   return status ();
}

static esp_err_t
web_root (httpd_req_t * req)
{
   if (revk_link_down ())
      return revk_web_settings (req);   // Direct to web set up
   revk_web_head (req, "ASR33");
   revk_web_send (req, "<script>"       //
                  "var ws=0;"   //
                  "var reboot=0;"       //
                  "function g(n){return document.getElementById(n);};"  //
                  "function s(n,v){var d=g(n);if(d)d.textContent=v;}"   //
                  "function w(n,v){var m=new Object();m[n]=v;ws.send(JSON.stringify(m));}"      //
                  "function c(){"       //
                  "ws=new WebSocket((location.protocol=='https:'?'wss:':'ws:')+'//'+window.location.host+'/status');"   //
                  "ws.onclose=function(v){ws=undefined;if(reboot)location.reload();};"  //
                  "ws.onerror=function(v){ws.close();};"        //
                  "ws.onmessage=function(v){"   //
                  "o=JSON.parse(v.data);"       //
                  "if(o.shutdown){reboot=true;s('shutdown','Restarting: '+o.shutdown);};"       //
                  "s('stats','Tx:'+o.tx+' Rx:'+o.rx+(o.rxlevel?'(1)':'(0)')+' Bad: Start:'+o.rxbadstart+' Stop:'+o.rxbadstop+' Zero:'+o.rxbad0+'/'+o.rxbadish0+' One:'+o.rxbad1+'/'+o.rxbadish1+' Parity:'+o.rxbadp+(o.brk?' BREAK':'')+(o.power?' (power on)':''));"   //
                  "if(o.data)g('rx').append(o.data);"   //
                  "};};c();"    //
                  "setInterval(function() {if(!ws)c();else ws.send('');},1000);"        //
                  "</script><form name=f><h1>ASR33 %s</h1>"     //
                  "<p id=shutdown style='color:red;'></p>"      //
                  "<textarea style='font-family:monospace' rows=4 cols=80 name=text></textarea><br>"    //
                  "<input type=button value='Text' onclick='w(\"text\",f.text.value);'>"        //
                  "<input type=button value='Tape' onclick='w(\"tape\",f.text.value);'>"        //
                  "<input type=button value='WRU' onclick='w(\"wru\",true);'>"  //
                  "<input type=button value='Clear stats' onclick='w(\"clear\",true);'>"        //
                  "<p id=stats></p>"    //
                  "<pre id=rx style='border:1px solid blue;'></pre>"    //
                  "</form>"     //
                  , hostname);
   return revk_web_foot (req, 0, 1, NULL);
}

void
app_main (void)
{
   rxws_mutex = xSemaphoreCreateBinary ();
   xSemaphoreGive (rxws_mutex);
   revk_boot (&app_callback);
   revk_start ();
   {                            // Web interface
      httpd_config_t config = HTTPD_DEFAULT_CONFIG ();  // When updating the code below, make sure this is enough
      //  Note that we 're also 4 adding revk' s web config handlers
      config.max_uri_handlers = 8;
      if (!httpd_start (&webserver, &config))
      {
         revk_web_settings_add (webserver);
         register_get_uri ("/", web_root);
         register_ws_uri ("/status", web_live);
      }
   }
   TaskHandle_t task_id = NULL;
   xTaskCreatePinnedToCore (asr33_main, "asr33", 20 * 1024, NULL, 2, &task_id, 1);
}
