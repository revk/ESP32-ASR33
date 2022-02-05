// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0

#include "revk.h"
#include <esp_spi_flash.h>
#include <driver/gpio.h>
#include "tty.h"

#define	EOT	4
#define	WRU	5
#define	RU	6
#define	DC1	0x11
#define	DC2	0x12
#define	DC3	0x13
#define	DC4	0x14

#define settings  \
  io(tx,4)      \
  io(rx,-5)     \
  io(run,21)  \
  io(pwr,23)      \
  io(mtr,22)  \
  u1(txod)	\
  u1(txpu)	\
  u1t(rxpu)	\
  t(pwrtopic)	\
  t(mtrtopic)	\
  u1(noecho)	\
  u1(nobig)	\
  u1(nover)	\
  t(wru)	\
  u32(idle,1)	\
  u32(keyidle,600)	\
  u8(ack,6)	\
  u8(think,10)	\
  u1(nocave)	\
  u1(cave)	\
  u1(nodc4)	\
  u8(tapelead,15) \
  u8(tapetail,15) \
  u16(port,33)	\
  u16(baudx100,11000)	\
  u8(databits,8)	\
  u8(stopx2,4)	\

#define u32(n,d) uint32_t n;
#define u16(n,d) uint16_t n;
#define u8(n,d) uint8_t n;
#define s8(n,d) int8_t n;
#define io(n,d) uint8_t n;
#define u1(n) uint8_t n;
#define u1t(n) uint8_t n;
#define t(n) const char*n=NULL;
settings;
#undef t
#undef u32
#undef u16
#undef u8
#undef s8
#undef io
#undef u1
#undef u1t
#define	MAXRX	256             // Line max

#define	LINELEN	72              // ASR33 line len

volatile int8_t manual = 0;     // Manual power override (eg key pressed, etc)
int8_t busy = 0;                // Busy state
uint8_t brk = 0;                // Break
uint8_t pressed = 0;            // Button pressed
uint8_t xoff = 0;               // Manual no echo
uint8_t doecho = 0;             // Do echo (set each start up)
uint8_t dobig = 0;              // Do big lettering on tape
uint8_t docave = 0;             // Fun advent()
uint8_t suppress = 0;           // Suppress WRU
int lsock = -1;                 // Listen socket
int csock = -1;                 // Connected sockets
volatile uint8_t havepower = 0; // Power state
volatile uint8_t wantpower = 0; // Power state wanted
char line[MAXRX + 1];           // Rx line buffer
int64_t lastrx = 0;             // Last rx
int64_t done = 0;               // When to turn off
int8_t pos = -1;                // Position (-1 is unknown)
uint32_t rxp = 0;               // Rx line buffer pointer
uint8_t hayes = 0;              // Hayes +++ counter

const unsigned char small_f[256][5] = {
#include "smallfont.h"
};

static inline uint8_t pe(uint8_t b)
{                               // Make even parity
   b &= 0x7F;
   for (int i = 0; i < 7; i++)
      if (b & (1 << i))
         b ^= 0x80;
   return b;
}

void sendbyte(uint8_t b)
{
   tty_tx(b);
   b &= 0x7F;
   if (b == '\r')
      pos = 0;
   else if (b >= ' ' && b < 0x7F && pos < LINELEN)
      pos++;
}

void cr(void)
{                               // Do a carriage return
   int8_t p = pos;
   if (p)
   {
      sendbyte(pe('\r'));       // CR
      if (p < 0 || p > 40)
         sendbyte(0);           // allow extra time for CR
      if (p < 0)
         sendbyte(0);           // allow extra time for CR
   }
}

void nl(void)
{                               // Do new line
   if (pos)
      cr();                     // CR
   sendbyte(pe('\n'));          // LF (allows time for CR as well)
}


void sendstring(const char *c)
{                               // Simple text string, with wrap and \n for line breaks
   if (c)
      while (*c)
      {
         if (*c == '\n')
            nl();
         else
         {
            if (pos == 72)
               nl();
            sendbyte(pe(*c));
         }
         c++;
      }
}

int queuebig(int c)
{                               // Send a big character for tape
   if (c >= sizeof(small_f) / sizeof(&small_f))
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
      sendbyte(c);
      c &= 0x7f;
      if (!nodc4 && c == DC4)
         sendbyte(DC2);
      if (c == WRU)
      {
         suppress = 1;
         lastrx = 0;
      }
   }
   return 1;
}

void reportstate(void)
{
   uint64_t t = esp_timer_get_time();
   jo_t j = jo_create_alloc();
   jo_object(j, NULL);
   jo_litf(j, "up", "%d.%06d", (uint32_t) (t / 1000000LL), (uint32_t) (t % 1000000LL));
   jo_bool(j, "power", havepower);
   jo_bool(j, "break", brk);
   jo_bool(j, "busy", busy);
   if (port)
      jo_bool(j, "connected", csock >= 0);
   revk_state(NULL, &j);
}

void power_off(void)
{
   if (havepower == 0)
      return;
   manual = 0;
   rxp = 0;
   hayes = 0;
   doecho = !noecho;
   xoff = 0;
   havepower = 0;
   reportstate();
   tty_flush();
   if (mtr || *mtrtopic)
   {                            // Motor direct control, off
      usleep(100000);           // If final character being printed...
      if (*mtrtopic)
         revk_mqtt_send_raw(mtrtopic, 0, "0", 0);
      if (mtr)
         gpio_set_level(port_mask(mtr), port_inv(mtr)); // Off
      sleep(1);                 // Takes time for motor to spin down - ensure this is done before power goes off
   } else
      pos = -1;                 // No direct motor control, assume gash character(s) so pos unknown
   if (pwr || *pwrtopic)
   {
      if (*pwrtopic)
         revk_mqtt_send_raw(pwrtopic, 0, "0", 0);
      if (pwr)
         gpio_set_level(port_mask(pwr), port_inv(pwr)); // Off
   }
}

void power_on(void)
{
   if (havepower == 1)
      return;
   if (pwr || *pwrtopic)
   {                            // Power, direct control, on
      if (*pwrtopic)
         revk_mqtt_send_raw(pwrtopic, 0, "1", 0);
      if (pwr)
         gpio_set_level(port_mask(pwr), 1 - port_inv(pwr));     // On
      usleep(100000);           // Min is 20ms for zero crossing, and 9ms for one bit for solenoid, but can be longer for safety
   }
   if (mtr || *mtrtopic)
   {                            // Motor, direct control, on
      if (*mtrtopic)
         revk_mqtt_send_raw(mtrtopic, 0, "1", 0);
      if (mtr)
         gpio_set_level(port_mask(mtr), 1 - port_inv(mtr));     // On
      usleep(250000);           // Min 100ms for a null character from power off, and some for motor to start and get to speed
   }
   havepower = 1;
   reportstate();
   tty_flush();
   done = esp_timer_get_time() + 1000000 * (manual ? keyidle : idle);
}

void power_needed(void)
{                               // Power on if needed, queue \r
   if (wantpower)
      return;                   // Power already on
   if (pos < 0)
      cr();                     // Move to known place
   wantpower = 1;
}

const char *app_callback(int client, const char *prefix, const char *target, const char *suffix, jo_t j)
{
   if (client || target || !prefix || strcmp(prefix, "command"))
      return NULL;              // Not what we want
   if (!strcmp(suffix, "status"))
      reportstate();
   if (!strcmp(suffix, "connect"))
   {
      if (*pwrtopic)
         revk_mqtt_send_raw(pwrtopic, 0, havepower ? "1" : "0", 0);
      if (*mtrtopic)
         revk_mqtt_send_raw(mtrtopic, 0, havepower ? "1" : "0", 0);
      reportstate();
   }
   if (!strcmp(suffix, "restart"))
      wantpower = 0;
   if (!strcmp(suffix, "cave"))
      docave = 1;
   if (!strcmp(suffix, "on"))
   {
      manual = 1;
      power_needed();
   }
   if (!strcmp(suffix, "off"))
   {
      wantpower = 0;
      manual = 0;
      done = 0;
   }
   if (!strcmp(suffix, "echo"))
      doecho = 1;
   if (!strcmp(suffix, "noecho"))
      doecho = 0;
   if (!strcmp(suffix, "break"))
      tty_break();

   if (!strcmp(suffix, "tape") || !strcmp(suffix, "taperaw") || !strcmp(suffix, "text") || !strcmp(suffix, "line") || !strcmp(suffix, "bell"))
   {                            // Plain text functions - simple JSON string
      int len = jo_strlen(j);
      if (len < 0)
         return "JSON string expected";
      char *buf = jo_strdup(j),
          *value = buf;
      if (!buf)
         return "Malloc";
      if (!strcmp(suffix, "tape") || !strcmp(suffix, "taperaw"))
      {                         // Punched tape text
         power_needed();
         if (!suffix[4])
         {
            if (!nodc4)
               sendbyte(DC2);   // Tape on
            for (int i = 0; i < tapelead; i++)
               sendbyte(0);
         }
         if (!strcmp(suffix, "taperaw"))
            while (len--)
               sendbyte(*value++);      // Raw
         else                   // Large text
            while (len--)
            {
               if (!queuebig(*value++))
                  continue;
               if (len)
                  sendbyte(0);
            }
         if (!suffix[4])
         {
            for (int i = 0; i < tapetail; i++)
               sendbyte(0);
            if (!nodc4)
               sendbyte(DC4);   // Tape off
         }
      }
      if (!strcmp(suffix, "text") || !strcmp(suffix, "line") || !strcmp(suffix, "bell"))
      {
         power_needed();
         while (len--)
         {
            uint32_t b = *value++;
            // We assume this is utf-8
            if ((b & 0xC0) == 0x80)
               b = 0x7F;        // Silly, not even utf-8, rub out
            else if ((b & 0xF8) == 0xF0 && len >= 3 && (value[0] & 0xC0) == 0x80 && (value[1] & 0xC0) == 0x80 && (value[2] & 0xC0) == 0x80)
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
               b = 'A' + b - 0x1D670;   // Maths monospace characters
            else if (b >= 0x1D68A && b < 0x1D68A + 26)
               b = 'a' + b - 0x1D68A;
            else if (b >= 0x1D7F6 && b < 0x1D7F6 + 10)
               b = '0' + b - 0x1D7F6;
            else if (b == 0x2014)
               b = '-';         // Common hyphen (long)
            else if (b == 0x2018 || b == 0x2019)
               b = '\'';        // Common quote escape
            else if (b == 0x201c || b == 0x201d)
               b = '"';         // Common quote escape
            if (b >= 0x80)
               b = 0x5E;        // Other unicode so print as Up arrow
            if (b >= ' ' && b < 0x7F && pos >= LINELEN)
               nl();            // Force newline as would overprint
            if (b == '\n')
               nl();            // We want a new line (does CR too)
            else if (b == '\r')
            {                   // Explicit CR, let's assume no NL
               cr();            // We want a carriage return
               sendbyte(0);     // Assuming no LF, need extra null
            } else
               sendbyte(pe(b));
         }
         if (!strcmp(suffix, "line") || !strcmp(suffix, "bell"))
         {
            cr();
            nl();
            if (!strcmp(suffix, "bell"))
               sendbyte(pe(7));
         }
      }
      return "";
   }
   if (!strcmp(suffix, "tx") || !strcmp(suffix, "txraw") || !strcmp(suffix, "raw") || !strcmp(suffix, "punch") || !strcmp(suffix, "punchraw"))
   {
      // Functions that expect hex data
      int len = jo_strncpy(j, NULL, 0);
      if (len < 0)
         return "Expecting JSON string";

      char *buf = malloc(len),
          *value = buf;
      if (!buf)
         return "Malloc";
      jo_strncpy(j, buf, len);
      if (!strcmp(suffix, "tx") || !strcmp(suffix, "txraw") || !strcmp(suffix, "raw"))
      {                         // raw send
         power_needed();
         while (len--)
            sendbyte(*value++);
      }
      if (!strcmp(suffix, "punch") || !strcmp(suffix, "punchraw"))
      {                         // Raw punched data (with DC2/DC4)
         power_needed();
         if (!nodc4)
            sendbyte(DC2);      // Tape on
         if (!suffix[5])
         {
            for (int i = 0; i < tapelead; i++)
               sendbyte(0);
         }
         while (len--)
         {
            uint8_t c = *value++;
            sendbyte(c);
            c &= 0x7f;
            if (!nodc4 && c == DC4)
               sendbyte(DC2);   // Turn tape back on
            if (c == WRU)
            {
               suppress = 1;    // Suppress WRU response
               lastrx = 0;
            }
         }
         if (!suffix[5])
         {
            for (int i = 0; i < tapetail; i++)
               sendbyte(0);
         }
         if (!nodc4)
            sendbyte(DC4);      // Tape off
      }
      free(buf);
   }
   return "";
}

void asr33_main(void *param)
{
   revk_boot(&app_callback);
#define u32(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u16(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u8(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define s8(n,d) revk_register(#n,0,sizeof(n),&n,#d,SETTING_SIGNED);
#define io(n,d) revk_register(#n,0,sizeof(n),&n,"- "#d,SETTING_SET|SETTING_BITFIELD);
#define u1(n) revk_register(#n,0,sizeof(n),&n,NULL,SETTING_BOOLEAN);
#define u1t(n) revk_register(#n,0,sizeof(n),&n,"1",SETTING_BOOLEAN);
#define t(n) revk_register(#n,0,0,&n,NULL,0);
   settings;
#undef t
#undef u32
#undef u16
#undef u8
#undef s8
#undef io
#undef u1
#undef u1t
   revk_start();
   doecho = !noecho;

   tty_setup();

   if (run)
   {                            // RUN input
      gpio_set_direction(port_mask(run), GPIO_MODE_INPUT);
      gpio_set_pull_mode(port_mask(run), GPIO_PULLUP_ONLY);
      gpio_pullup_en(port_mask(run));
   }
   if (pwr)
   {                            // PWR output
      gpio_set_level(port_mask(pwr), port_inv(pwr));    // Off
      gpio_set_direction(port_mask(pwr), GPIO_MODE_OUTPUT);
   }
   if (mtr)
   {                            // MTR output
      gpio_set_level(port_mask(mtr), port_inv(mtr));    // Off
      gpio_set_direction(port_mask(mtr), GPIO_MODE_OUTPUT);
   }
   if (port)
      lsock = socket(AF_INET6, SOCK_STREAM, IPPROTO_IPV6);
   if (lsock >= 0)
   {
      struct sockaddr_storage dest_addr;
      struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *) &dest_addr;
      bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
      dest_addr_ip6->sin6_family = AF_INET6;
      dest_addr_ip6->sin6_port = htons(port);
      if (bind(lsock, (struct sockaddr *) &dest_addr, sizeof(dest_addr)))
      {
         close(lsock);
         lsock = -1;
      }
   }
   if (lsock >= 0 && listen(lsock, 1))
   {
      close(lsock);
      lsock = -1;
   }
   while (1)
   {
      usleep(10000);
      int64_t now = esp_timer_get_time();
      int64_t gap = now - lastrx;
      if (run && gpio_get_level(port_mask(run)) != port_inv(run))
      {                         // RUN button
         if (!pressed)
         {                      // Button pressed
            pressed = 1;
            manual = 1;
            if (!wantpower)
            {
               power_needed();
               if (cave)
                  docave = 1;   // Only playing a game
            } else
               wantpower = 0;
            {
               jo_t j = jo_create_alloc();
               jo_string(j, NULL, wantpower ? "on" : "off");
               revk_event(NULL, &j);
            }
            if (!wantpower && csock >= 0)
            {
               close(csock);
               csock = -1;
               jo_t j = jo_object_alloc();
               jo_string(j, "reason", "run/stop");
               revk_event("closed", &j);
            }
         }
      } else
         pressed = 0;
      // Check tx buffer usage
      if (tty_tx_space() < 4096)
      {
         if (!busy)
         {
            busy = 1;
            reportstate();
         }
      } else
      {
         if (busy)
         {
            busy = 0;
            reportstate();
         }
      }
      if (tty_tx_waiting() || csock >= 0)
         wantpower = 1;
      if (lsock >= 0 && csock < 0)
      {                         // Allow for connection
         fd_set s;
         FD_ZERO(&s);
         FD_SET(lsock, &s);
         struct timeval timeout = { };
         if (select(lsock + 1, &s, NULL, NULL, &timeout) > 0)
         {
            struct sockaddr_storage source_addr;        // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);
            csock = accept(lsock, (struct sockaddr *) &source_addr, &addr_len);
            char addr_str[40] = "";
            if (source_addr.ss_family == PF_INET)
               inet_ntoa_r(((struct sockaddr_in *) &source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            else if (source_addr.ss_family == PF_INET6)
               inet6_ntoa_r(((struct sockaddr_in6 *) &source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
            jo_t j = jo_object_alloc();
            jo_string(j, "ip", addr_str);
            revk_event("connect", &j);
            power_needed();
            pos = -1;
         }
      }
      if (wantpower != havepower)
      {                         // Change power state (does any necessary timing sequence)
         if (wantpower == 0)
            power_off();
         else if (wantpower == 1)
            power_on();
      }
      if (hayes == 3 && gap > 1000000)
      {                         // End of Hayes +++ escape sequence
         hayes++;               // Command prompt
         rxp = 0;
         sendstring("\nASR33 CONTROLLER (BUILD ");
         sendstring(revk_version);
         sendstring(") ASR33.REVK.UK\n");
         if (revk_link_down())
         {
            sendstring("OFFLINE (SSID ");
            sendstring(revk_wifi());
            sendstring(")\n");
         } else if (port)
         {                      // Online
            sendstring("LISTENING ON ");
            char temp[50];
            ip6_addr_t ip6;
            if (!tcpip_adapter_get_ip6_global(TCPIP_ADAPTER_IF_STA, &ip6))
            {
               inet6_ntoa_r(ip6, temp, sizeof(temp) - 1);
               sendstring(temp);
               sendstring(" & ");
            }
            tcpip_adapter_ip_info_t ip4;
            if (!tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip4))
            {
               inet_ntoa_r(ip4.ip, temp, sizeof(temp) - 1);
               sendstring(temp);
            }
            sendstring("\nENTER IP/DOMAIN TO MAKE CONNECTION");
            sprintf(temp, " (USING TCP PORT %d)", port);
            sendstring(temp);
            sendstring("\nOR ");
         }
         if (!nocave)
            sendstring("WOULD YOU LIKE TO PLAY A GAME? (Y/N)\n> ");
      }
      // Check tcp
      if (csock >= 0)
      {
         fd_set s;
         FD_ZERO(&s);
         FD_SET(csock, &s);
         struct timeval timeout = { };
         if (select(csock + 1, &s, NULL, NULL, &timeout) > 0)
         {
            uint8_t b = 0;
            int len = recv(csock, &b, 1, 0);
            if (len <= 0)
            {                   // Closed
               close(csock);
               csock = -1;
               jo_t j = jo_object_alloc();
               jo_string(j, "reason", "close");
               revk_event("closed", &j);
            } else
               tty_tx(b);       // Raw send to teletype
         }
      }
      int len = tty_rx_ready();
      if (!len)
      {
         if (brk)
         {
            brk = 0;
            reportstate();
         }
      } else if (len < 0)
      {                         // Break
         if (!brk)
         {
            brk = 1;
            hayes = 0;
            rxp = 0;
            reportstate();
            if (csock >= 0)
            {
               close(csock);
               csock = -1;
               jo_t j = jo_object_alloc();
               jo_string(j, "reason", "break");
               revk_event("closed", &j);
               manual = 0;
            }
            power_needed();     // Power on BREAK
         }
      } else if (len > 0)
      {
         uint8_t b = tty_rx();
         if (csock >= 0)
            send(csock, &b, 1, 0);      // Connected via TCP
         else
         {                      // Not connected via TCP
            if (gap > 250000)
               suppress = 0;    // Suppressed WRU response timeout can end
            if (hayes > 3)
            {                   // Hayes command prompt only
               if (b == pe('\r') || b == pe('\n'))
               {                // Command
                  sendstring("\n");
                  line[rxp] = 0;
                  hayes = 0;
                  rxp = 0;
                  if (!strcmp(line, "Y") || !strcmp(line, "YES"))
                     docave = 1;
                  else if (!strcmp(line, "N") || !strcmp(line, "NO"))
                     sendstring("SHAME, BYE\n");
                  else if (*line)
                  {             // DNS/IP?
                     const struct addrinfo hints = {
                        .ai_family = AF_UNSPEC,
                        .ai_socktype = SOCK_STREAM,
                        .ai_flags = AI_CANONNAME,
                     };
                     char ports[20];
                     sprintf(ports, "%d", port);
                     struct addrinfo *res = NULL,
                         *a;
                     int err = getaddrinfo(line, ports, &hints, &res);
                     if (err || !res)
                        sendstring("+++ HOST NAME NOT FOUND +++\n");
                     else
                     {
                        for (a = res; a; a = a->ai_next)
                        {
                           int s = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
                           if (s >= 0)
                           {
                              if (!connect(s, a->ai_addr, a->ai_addrlen))
                              {
                                 csock = s;
                                 sendstring("+++ CONNECTED ");
                                 if (res->ai_canonname)
                                    sendstring(res->ai_canonname);
                                 sendstring(" +++\n");
                                 jo_t j = jo_object_alloc();
                                 if (res && res->ai_canonname)
                                    jo_string(j, "target", res->ai_canonname);
                                 revk_event("connect", &j);
                                 pos = -1;
                                 break;
                              }
                              close(s);
                           }
                        }
                        if (csock < 0)
                        {
                           sendstring("+++ COULD NOT CONNECT ");
                           if (res && res->ai_canonname)
                              sendstring(res->ai_canonname);
                           sendstring(" +++\n");
                        }
                        freeaddrinfo(res);
                     }
                  }
               } else if ((b & 0x7f) > ' ' && rxp < MAXRX)
               {
                  sendbyte(b);  // echo
                  line[rxp++] = (b & 0x7F);
               }
            } else
            {
               if (!hayes)
               {
                  if (b == pe('+') && !rxp && gap > 1000000)
                     hayes++;
               } else
               {
                  if (b != pe('+') || hayes >= 3 || gap > 1000000)
                     hayes = 0;
                  else
                     hayes++;
               }
               if (!suppress)
               {
                  if (b && b != 0xFF && b == pe(b))
                  {
                     if (b == pe(EOT))
                     {
                        manual = 0;     // EOT, short timeout
                        done = 0;
                     } else
                        manual = 1;     // Typing
                  }
                  jo_t j = jo_object_alloc();
                  jo_int(j, "byte", b);
                  revk_event("rx", &j);
                  if ((b & 0x7F) == '\n' || (b & 0x7F) == '\r')
                  {
                     jo_t j = jo_create_alloc();
                     jo_stringn(j, NULL, (void *) line, rxp);
                     revk_event("line", &j);
                     rxp = 0;
                  } else if ((b & 0x7f) > ' ' && rxp < MAXRX)
                     line[rxp++] = (b & 0x7F);
                  if (doecho)
                  {             // Handling local characters and echoing (maybe, depends on xoff too)
                     if (dobig)
                     {          // Doing large lettering
                        if (b == pe(DC4))
                        {       // End
                           for (int i = 0; i < 9; i++)
                              sendbyte(0);
                           sendbyte(pe(DC4));
                           dobig = 0;
                        } else if (b == pe(b) && (b & 0x7F) >= 0x20 && queuebig(b & 0x7F))
                           sendbyte(0);
                     } else if (b == pe(DC2) && doecho && !nobig)
                     {          // Start big lettering
                        sendbyte(pe(DC2));
                        for (int i = 0; i < 10; i++)
                           sendbyte(0);
                        dobig = 1;
                     }
                     // else if (b == pe(RU) && !nocave) docave = 1;
                     else if (b == pe(DC1))
                        xoff = 0;
                     else if (b == pe(DC3))
                        xoff = 1;
                     else if (b == pe(WRU) && (!nover || *wru))
                     {          // WRU
                        // See 3.27 of ISS 8, SECTION 574-122-700TC
                        sendbyte(pe('\r'));     // CR
                        sendbyte(pe('\n'));     // LF
                        sendbyte(pe(0x7F));     // RO
                        sendstring(wru);
                        if (!nover)
                        {
                           if (*wru)
                              sendbyte(pe(' '));
                           sendstring(revk_app);
                           sendstring(" BUILD ");
                           sendstring(revk_version);
                        }
                        sendbyte(pe('\r'));     // CR
                        sendbyte(pe('\n'));     // LF
                        if (ack)
                           sendbyte(pe(ack));   // ACK
                     } else if (!xoff)
                        sendbyte(b);
                  }
               }
            }
         }
         lastrx = now;
      }
      if (!tty_tx_waiting())
      {                         // Nothing to send
         if (docave)
         {                      // Let's play a game
            docave = 0;
            extern int advent(void);
            if (!busy)
            {
               busy = 1;
               reportstate();
            }
            advent();
            manual = 0;
            done = 0;
         } else if (now > done)
            wantpower = 0;      // Idle complete, power off
      } else
         done = now + 1000000 * (manual ? keyidle : idle);
   }
}

void app_main(void)
{
   TaskHandle_t task_id = NULL;
   xTaskCreatePinnedToCore(asr33_main, "asr33", 20 * 1024, NULL, 2, &task_id, 1);
}
