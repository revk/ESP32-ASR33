// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0

#include "revk.h"
#include <esp_spi_flash.h>
#include <driver/gpio.h>
#include "softuart.h"
#include "tty.h"

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
  u1(autocave)	\
  u1(autoon)	\
  u1(autoprompt)	\
  u1(nodc4)	\
  u8(tapelead,15) \
  u8(tapetail,15) \
  u16(port,33)	\
  u16(baudx100,11000)	\
  u8(databits,8)	\
  u8(stopx2,4)	\
  u8(linelen,72)	\
  u16(crms,200)		\

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
   if (b == CR)
      pos = 0;
   else if (b >= ' ' && b < RO)
      pos++;
}

void cr(void)
{                               // Do a carriage return
   sendbyte(pe(CR));
}

void nl(void)
{                               // Do new line
   cr();
   sendbyte(pe(LF));
}

void sendstring(const char *c)
{                               // Simple text string, with wrap and \n for line breaks
   if (c)
      while (*c)
      {
         if (*c == LF)
            nl();
         else
         {
            if (pos >= linelen)
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
   jo_t j = jo_object_alloc();
   jo_litf(j, "up", "%d.%06d", (uint32_t) (t / 1000000LL), (uint32_t) (t % 1000000LL));
   jo_bool(j, "power", on);
   jo_bool(j, "break", brk);
   jo_bool(j, "busy", busy);
   if (port)
      jo_bool(j, "connected", csock >= 0);
   revk_state(NULL, &j);
}

void power_off(void)
{
   if (!on)
      return;                   // Already off
   power = 0;
   rxp = 0;
   hayes = 0;
   doecho = !noecho;
   xoff = 0;
   reportstate();
   tty_flush();
   tty_xoff();
   usleep(100000);
   if (mtr || *mtrtopic)
   {                            // Motor direct control, off
      if (*mtrtopic)
         revk_mqtt_send_raw(mtrtopic, 0, "0", 0);
      if (mtr)
         gpio_set_level(port_mask(mtr), port_inv(mtr)); // Off
      sleep(1);                 // Takes time for motor to spin down - ensure this is done before power goes off
   }
   if (pwr || *pwrtopic)
   {
      if (*pwrtopic)
         revk_mqtt_send_raw(pwrtopic, 0, "0", 0);
      if (pwr)
         gpio_set_level(port_mask(pwr), port_inv(pwr)); // Off
      sleep(1);
   }
   on = 0;
   reportstate();
}

void power_on(void)
{
   done = esp_timer_get_time() + 1000000 * (power > 1 ? keyidle : idle);
   if (on)
      return;                   // Already on
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
   on = 1;
   reportstate();
   tty_xon();
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
         revk_mqtt_send_raw(pwrtopic, 0, on ? "1" : "0", 0);
      if (*mtrtopic)
         revk_mqtt_send_raw(mtrtopic, 0, on ? "1" : "0", 0);
      reportstate();
   }
   if (!strcmp(suffix, "restart"))
      power = -1;
   if (!strcmp(suffix, "cave"))
      docave = 1;
   if (!strcmp(suffix, "on"))
      power = 2;
   if (!strcmp(suffix, "off"))
      power = -1;
   if (!strcmp(suffix, "echo"))
      doecho = 1;
   if (!strcmp(suffix, "noecho"))
      doecho = 0;
   if (!strcmp(suffix, "break"))
   {
      power = 1;
      int chars = 10;
      if (j && jo_here(j) == JO_NUMBER)
         chars = jo_read_int(j);
      tty_break(chars);
   }
   if (!strcmp(suffix, "uartstats"))
   {
      softuart_stats_t s;
      tty_stats(&s);
      jo_t j = jo_object_alloc();
      jo_int(j, "tx", s.tx);
      jo_int(j, "rx", s.rx);
      jo_int(j, "rxbadstart", s.rxbadstart);
      jo_int(j, "rxbadstop", s.rxbadstop);
      jo_int(j, "rxbad0", s.rxbad0);
      jo_int(j, "rxbad1", s.rxbad1);
      revk_info("uartstats", &j);
   }

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
         power = 1;
         if (!suffix[4])
         {
            if (!nodc4)
               sendbyte(DC2);   // Tape on
            for (int i = 0; i < tapelead; i++)
               sendbyte(NUL);
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
                  sendbyte(NUL);
            }
         if (!suffix[4])
         {
            for (int i = 0; i < tapetail; i++)
               sendbyte(NUL);
            if (!nodc4)
               sendbyte(DC4);   // Tape off
         }
      }
      if (!strcmp(suffix, "text") || !strcmp(suffix, "line") || !strcmp(suffix, "bell"))
      {
         power = 1;
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
            if (b >= ' ' && b < 0x7F && pos >= linelen)
               nl();            // Force newline as would overprint
            if (b == LF)
               nl();            // We want a new line (does CR too)
            else if (b == CR)
            {                   // Explicit CR, let's assume no NL
               cr();            // We want a carriage return
               sendbyte(NUL);   // Assuming no LF, need extra null
            } else
               sendbyte(pe(b));
         }
         if (!strcmp(suffix, "line") || !strcmp(suffix, "bell"))
         {
            cr();
            nl();
            if (!strcmp(suffix, "bell"))
               sendbyte(pe(BEL));
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
         power = 1;
         while (len--)
            sendbyte(*value++);
      }
      if (!strcmp(suffix, "punch") || !strcmp(suffix, "punchraw"))
      {                         // Raw punched data (with DC2/DC4)
         power = 1;
         if (!nodc4)
            sendbyte(DC2);      // Tape on
         if (!suffix[5])
         {
            for (int i = 0; i < tapelead; i++)
               sendbyte(NUL);
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
               sendbyte(NUL);
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
      lsock = socket(AF_INET6, SOCK_STREAM, 0); // IPPROTO_IPV6);
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
   tty_xoff();
   void dorun(void) {           // RUN button manual start
      power = 2;
      if (autoprompt)
         hayes = 3;
      else if (autocave)
         docave = 1;
   }
   if (autoon)
      dorun();
   while (1)
   {
      usleep(10000);
      int64_t now = esp_timer_get_time();
      int64_t gap = now - lastrx;
      // Handle RUN button
      if (run && gpio_get_level(port_mask(run)) != port_inv(run))
      {                         // RUN button
         if (!pressed)
         {                      // Button pressed
            pressed = 1;
            if (on)
               power = -1;      // Turn off
            else
               dorun();
         }
      } else
         pressed = 0;
      // Handle power change
      if (power < 0)
      {                         // Power off
         if (on && !tty_tx_waiting())
         {                      // Do power off once tx done
            if (csock >= 0)
            {                   // Close connection
               close(csock);
               csock = -1;
               jo_t j = jo_object_alloc();
               jo_string(j, "reason", "power");
               revk_event("closed", &j);
            }
            power_off();
         }
      } else if (power)
      {
         if (!on)
            power_on();
      }
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
      // Handle incoming connection
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
            power = 1;
         }
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
            sendstring("\n");
            if (!nocave)
               sendstring("OR ");
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
               sendbyte(b);     // Raw send to teletype
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
               power = -1;
            }
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
               if (b == pe(CR) || b == pe(LF))
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
               } else if ((b & 0x7f) >= ' ' && rxp < MAXRX)
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
                  jo_t j = jo_object_alloc();
                  jo_int(j, "byte", b);
                  revk_event("rx", &j);
                  if (b == pe(EOT))
                     power = -1;        // EOT to shut down
                  if ((b & 0x7F) == LF || (b & 0x7F) == CR)
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
                              sendbyte(NUL);
                           sendbyte(pe(DC4));
                           dobig = 0;
                        } else if (b == pe(b) && (b & 0x7F) >= 0x20 && queuebig(b & 0x7F))
                           sendbyte(NUL);
                     } else if (b == pe(DC2) && doecho && !nobig)
                     {          // Start big lettering
                        sendbyte(pe(DC2));
                        for (int i = 0; i < 10; i++)
                           sendbyte(NUL);
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
                        sendbyte(pe(CR));       // CR
                        sendbyte(pe(LF));       // LF
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
                        sendbyte(pe(CR));       // CR
                        sendbyte(pe(LF));       // LF
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
      if (!tty_tx_waiting() && csock < 0)
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
            power = -1;
         } else if (now > done)
            power = -1;
      } else
         done = now + 1000000 * (power > 1 ? keyidle : idle);
   }
}

void app_main(void)
{
   TaskHandle_t task_id = NULL;
   xTaskCreatePinnedToCore(asr33_main, "asr33", 20 * 1024, NULL, 2, &task_id, 1);
}
