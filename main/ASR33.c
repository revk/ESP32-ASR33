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

volatile int8_t power = 0;      // power request, -1 means want off, 1 means want on, 2 means want on with long timeout
int8_t on = 0;                  // Power is on
int8_t busy = 0;                // Busy state (tx queue is low)
uint8_t brk = 0;                // Break condition
uint8_t pressed = 0;            // Button pressed
uint8_t xoff = 0;               // Manual no echo
uint8_t doecho = 0;             // Do echo (set each start up)
uint8_t dobig = 0;              // Do big lettering on tape
uint8_t docave = 0;             // Fun advent()
uint8_t suppress = 0;           // Suppress WRU
uint8_t pos = 0;                // Pos for wrapping
int lsock = -1;                 // Listen socket
int csock = -1;                 // Connected sockets
char line[MAXRX + 1];           // Rx line buffer
int64_t lastrx = 0;             // Last rx
int64_t done = 0;               // When to turn off
uint32_t rxp = 0;               // Rx line buffer pointer
uint8_t hayes = 0;              // Hayes +++ counter

static httpd_handle_t webserver = NULL;

const unsigned char small_f[256][5] = {
#include "smallfont.h"
};

static inline uint8_t
pe (uint8_t b)
{                               // Make even parity
   b &= 0x7F;
   for (int i = 0; i < 7; i++)
      if (b & (1 << i))
         b ^= 0x80;
   return b;
}

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
         suppress = 1;
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
   jo_bool (j, "power", on);
   jo_bool (j, "break", brk);
   jo_bool (j, "busy", busy);
   if (port)
      jo_bool (j, "connected", csock >= 0);
   revk_state (NULL, &j);
}

void
power_off (void)
{
   if (!on)
      return;                   // Already off
   power = 0;
   rxp = 0;
   hayes = 0;
   doecho = !noecho;
   xoff = 0;
   reportstate ();
   tty_flush ();
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
   on = 0;
   reportstate ();
}

void
power_on (void)
{
   if (!on)
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
      on = 1;
      reportstate ();
      tty_xon ();
   }
   done = esp_timer_get_time () + 1000 * (power > 1 ? timekeyidle : timeremidle);
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
         revk_mqtt_send_raw (pwrtopic, 0, on ? "1" : "0", 0);
      if (*mtrtopic)
         revk_mqtt_send_raw (mtrtopic, 0, on ? "1" : "0", 0);
      reportstate ();
   }
   if (!strcmp (suffix, "restart"))
      power = -1;
   if (!strcmp (suffix, "cave"))
      docave = 1;
   if (!strcmp (suffix, "on"))
      power = 2;
   if (!strcmp (suffix, "off"))
      power = -1;
   if (!strcmp (suffix, "echo"))
      doecho = 1;
   if (!strcmp (suffix, "noecho"))
      doecho = 0;
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
      softuart_stats_t s;
      tty_stats (&s);
      jo_t j = jo_object_alloc ();
      jo_int (j, "tx", s.tx);
      jo_int (j, "rx", s.rx);
      jo_int (j, "rxbadstart", s.rxbadstart);
      jo_int (j, "rxbadstop", s.rxbadstop);
      jo_int (j, "rxbad0", s.rxbad0);
      jo_int (j, "rxbad1", s.rxbad1);
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
               suppress = 1;    // Suppress WRU response
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
   doecho = !noecho;

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
   tty_xoff ();
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
      pressed = 1;              // Initial state for RUN/STOP button
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
            docave = 1;
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
      else if (on)
         revk_blink (1, 0, tty_tx_waiting ()? "RG" : "G");
      else
         revk_blink (0, 0, NULL);
      // Handle RUN button
      if (revk_gpio_get (run))
      {                         // RUN button
         if (!pressed)
         {                      // Button pressed
            pressed = 1;
            if (on)
               power = -1;      // Turn off
            else
               dorun ();
         }
      } else
         pressed = 0;
      // Handle power change
      if (power < 0)
      {                         // Power off
         if (on && !tty_tx_waiting ())
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
         if (!on)
            power_on ();
      }
      // Check tx buffer usage
      if (tty_tx_space () < 4096)
      {
         if (!busy)
         {
            busy = 1;
            reportstate ();
         }
      } else
      {
         if (busy)
         {
            busy = 0;
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
         if (brk)
         {                      // End of break
            brk = 0;
            reportstate ();
         }
      } else if (len < 0)
      {                         // Break
         if (!brk)
         {                      // Start of break
            brk = 1;
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
         if (!on)
            dorun ();           // Must be not using power controls, so turn on for rx data
         uint8_t b = tty_rx ();
         if (csock >= 0)
            send (csock, &b, 1, 0);     // Connected via TCP
         else
         {                      // Not connected via TCP
            if (gap > 250000)
               suppress = 0;    // Suppressed WRU response timeout can end
            if (hayes > 3)
            {                   // Hayes command prompt only
               if (b == pe (CR) || b == pe (LF))
               {                // Command
                  sendstring ("\n");
                  line[rxp] = 0;
                  hayes = 0;
                  rxp = 0;
                  if (!strcmp (line, "Y") || !strcmp (line, "YES"))
                     docave = 1;
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
               } else if ((b & 0x7f) >= ' ' && rxp < MAXRX)
               {
                  sendbyte (b); // echo
                  line[rxp++] = (b & 0x7F);
               }
            } else
            {
               if (!hayes)
               {
                  if (b == pe ('+') && !rxp && gap > 1000000)
                     hayes++;
               } else
               {
                  if (b != pe ('+') || hayes >= 3 || gap > 1000000)
                     hayes = 0;
                  else
                     hayes++;
               }
               if (!suppress)
               {
                  jo_t j = jo_object_alloc ();
                  jo_int (j, "byte", b);
                  revk_event ("rx", &j);
                  if (b == pe (EOT))
                     power = -1;        // EOT to shut down
                  if ((b & 0x7F) == LF || (b & 0x7F) == CR)
                  {
                     jo_t j = jo_create_alloc ();
                     jo_stringn (j, NULL, (void *) line, rxp);
                     revk_event ("line", &j);
                     rxp = 0;
                  } else if ((b & 0x7f) >= ' ' && rxp < MAXRX)
                     line[rxp++] = (b & 0x7F);
                  if (doecho)
                  {             // Handling local characters and echoing (maybe, depends on xoff too)
                     if (dobig)
                     {          // Doing large lettering
                        if (b == pe (DC4))
                        {       // End
                           for (int i = 0; i < 9; i++)
                              sendbyte (NUL);
                           sendbyte (pe (DC4));
                           dobig = 0;
                        } else if (b == pe (b) && (b & 0x7F) >= 0x20 && queuebig (b & 0x7F))
                           sendbyte (NUL);
                     } else if (b == pe (DC2) && doecho && !nobig)
                     {          // Start big lettering
                        sendbyte (pe (DC2));
                        for (int i = 0; i < 10; i++)
                           sendbyte (NUL);
                        dobig = 1;
                     }
                     // else if (b == pe(RU) && !nocave) docave = 1;
                     else if (b == pe (DC1))
                        xoff = 0;
                     else if (b == pe (DC3))
                        xoff = 1;
                     else if (b == pe (WRU) && (!nover || *wru))
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
                     } else if (!xoff)
                        sendbyte (b);
                  }
               }
            }
         }
         lastrx = now;
      }
      if (!tty_tx_waiting () && csock < 0)
      {                         // Nothing to send
         if (docave)
         {                      // Let's play a game
            docave = 0;
            extern int advent (void);
            if (!busy)
            {
               busy = 1;
               reportstate ();
            }
            revk_blink (1, 0, "B");
            advent ();
            power = -1;
         } else if (on && now > done)
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
register_post_uri (const char *uri, esp_err_t (*handler) (httpd_req_t * r))
{
   httpd_uri_t uri_struct = {
      .uri = uri,
      .method = HTTP_POST,
      .handler = handler,
   };
   register_uri (&uri_struct);
}

static esp_err_t
web_root (httpd_req_t * req)
{
   if (revk_link_down ())
      return revk_web_settings (req);   // Direct to web set up
   jo_t j = revk_web_query (req);
   revk_web_head (req, "ASR33");
   revk_web_send (req, "<h1>ASR33 %s</h1>", hostname);
   revk_web_send (req, "<table><form method=post><tr><td>");
   if (j && jo_find (j, "STAT"))
   {
      softuart_stats_t s;
      tty_stats (&s);
      revk_web_send (req, "tx=%d rx=%d rxbadstart=%d rxbadstop=%d rxbad0=%d rxbad1=%d", s.tx, s.rx, s.rxbadstart,
                     s.rxbadstop, s.rxbad0, s.rxbad1);
   }
   revk_web_send (req, "</td><td><input type=submit name=STAT Value='Stats'></td></tr></form>");
   revk_web_send (req,
                  "<form method=post><tr><td><textarea rows=4 cols=80 name=text></textarea></td><td><input type=submit name=TEXT value='Text'></td></tr></form>");
   revk_web_send (req, "<form method=post><tr><td><input size=80 name=tape></td><td><input type=submit name=TAPE value='Tape'></td></tr></form>");
   revk_web_send (req, "</table>");
   if (j)
   {
      if (jo_find (j, "TEXT") && jo_find (j, "text"))
      {                         // Text
         int len = jo_strlen (j);
         if (len > 0)
         {
            power = 1;
            char *text = jo_strdup (j);
            sendutf8 (len, text);
            nl ();
            free (text);
         }
      } else if (jo_find (j, "TAPE") && jo_find (j, "tape"))
      {                         // Punch big text
         int len = jo_strlen (j);
         if (len > 0)
         {
            power = 1;
            if (!nodc4)
               sendbyte (DC2);  // Tape on
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
               sendbyte (DC4);  // Tape off
               nl ();           // Tidy
            }
         }
      }
   }
   jo_free (&j);
   return revk_web_foot (req, 0, 1, NULL);
}

void
app_main (void)
{
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
         register_post_uri ("/", web_root);
      }
   }
   TaskHandle_t task_id = NULL;
   xTaskCreatePinnedToCore (asr33_main, "asr33", 20 * 1024, NULL, 2, &task_id, 1);
}
