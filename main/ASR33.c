// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0

#include "revk.h"
#include <esp_spi_flash.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#define	EOT	4
#define	WRU	5
#define	RU	6
#define	DC2	0x12
#define	DC4	0x14

#define settings  \
  u8(uart,1);	\
  u8(tx,17);	\
  u8(rx,16);	\
  u8(on,4);	\
  u8(pu,0xFF);	\
  u8(power,0xFF)\
  u8(motor,0xFF)\
  u1(echo);	\
  t(sonoff);	\
  t(wru);	\
  u1(ver);	\
  u32(wake,1)	\
  u32(idle,10)	\
  u32(keyidle,600)	\
  u8(ack,6)	\
  u8(think,10)	\
  u1(cave)	\
  u1(nodc4)	\
  u8(tapelead,15) \
  u8(tapetail,15) \

#define u32(n,d) uint32_t n;
#define u16(n,d) uint16_t n;
#define u8(n,d) uint8_t n;
#define u1(n) uint8_t n;
#define t(n) const char*n=NULL;
settings;
#undef t
#undef u32
#undef u16
#undef u8
#undef u1
#define	MAXTX	65536           // Tx buffer max
#define	MAXRX	256             // Line max

#define	LINELEN	72              // ASR33 line len

volatile int8_t manual = 0;     // Manual power override (eg key pressed, etc)
int8_t busy = 0;                // Busy state
uint8_t pressed = 0;            // Button pressed
uint8_t doecho = 0;             // Do echo (set each start up)
uint8_t docave = 0;             // Fun advent()
volatile uint8_t havepower = 0; // Power state
volatile uint8_t wantpower = 0; // Power state wanted
uint8_t buf[MAXTX];             // Tx pending buffer
volatile uint32_t txi = 0;      // Tx buffer in pointer
volatile uint32_t txo = 0;      // Tx buffer out pointer
uint8_t line[MAXRX];            // Rx line buffer
uint32_t rxp = 0;               // Rx line buffer pointer
int64_t eot = 0;                // When tx expected to end
volatile int64_t done = 0;      // When to next turn off
int8_t pos = -1;                // Position (-1 is unknown)
SemaphoreHandle_t queue_mutex = NULL;

const unsigned char small_f[256][5] = {
#include "smallfont.h"
};

inline uint8_t pe(uint8_t b)
{                               // Make even parity
   b &= 0x7F;
   for (int i = 0; i < 7; i++)
      if (b & (1 << i))
         b ^= 0x80;
   return b;
}

int64_t timeout(int extra)
{                               // Set power off timeout
   int64_t now = esp_timer_get_time();
   if (eot < now)
      eot = now;
   eot += extra;
   done = eot + 1000000 * (manual ? keyidle : idle);
   return now;
}

void queuebyte(uint8_t b)
{                               // Queue a byte
   xSemaphoreTake(queue_mutex, portMAX_DELAY);
   uint32_t n = txi + 1;
   if (n >= MAXTX)
      n = 0;
   if (n != txo)
   {                            // Not going too far
      buf[txi] = b;
      txi = n;
      // Position tracking
      b &= 0x7F;
      if (b == '\r')
         pos = 0;
      else if (b >= ' ' && b < 0x7F && pos < LINELEN)
         pos++;
   }
   xSemaphoreGive(queue_mutex);
   wantpower = 1;
}

void cr(void)
{                               // Do a carriage return
   int8_t p = pos;
   if (p)
   {
      queuebyte(pe('\r'));      // CR
      if (p < 0 || p > 40)
         queuebyte(0);          // allow extra time for CR
      if (p < 0)
         queuebyte(0);          // allow extra time for CR
   }
}

void nl(void)
{                               // Do new line
   if (pos)
      cr();                     // CR
   queuebyte(pe('\n'));         // LF (allows time for CR as well)
}

void power_off(void)
{
   if (havepower == 0)
      return;
   if (GPIO_IS_VALID_GPIO(motor))
   {                            // Motor direct control, off
      gpio_set_level(motor, 1);
      sleep(1);                 // Takes time for motor to spin down - ensure this is done before power goes off
   } else
      pos = -1;                 // No direct motor control, assume gash character(s) so pos unknown
   if (GPIO_IS_VALID_GPIO(power))
      gpio_set_level(power, 1);
   if (sonoff)
   {                            // Power, sonoff/mqtt, off
      revk_state("power", "%d", havepower = 0);
      revk_raw(NULL, sonoff, 1, "0", 0);
   }
   manual = 0;
   done = 0;
   txi = 0;
   txo = 0;
   rxp = 0;
   doecho = echo;
}

void power_on(void)
{
   if (havepower == 1)
      return;
   if (sonoff)
   {                            // Power, sonoff/mqtt, on
      revk_state("power", "%d", havepower = 1);
      revk_raw(NULL, sonoff, 1, "1", 0);
      sleep(1);                 // Allow time for MQTT, etc
   }
   if (GPIO_IS_VALID_GPIO(power))
   {                            // Power, direct control, on
      gpio_set_level(power, 0);
      usleep(100000);           // Min is 20ms for zero crossing, and 9ms for one bit for solenoid, but can be longer for safety
   }
   if (GPIO_IS_VALID_GPIO(motor))
   {                            // Motor, direct control, on
      gpio_set_level(motor, 0);
      usleep(200000);           // Min 100ms for a null character from power off, and some for motor to start and get to speed
   }
   uart_flush(uart);
   timeout(0);
}

void power_needed(void)
{                               // Power on if needed, queue \r
   if (wantpower)
      return;                   // Power already on
   if (pos < 0)
      cr();                     // Move to known place
}

const char *app_command(const char *tag, unsigned int len, const unsigned char *value)
{
   if (!strcmp(tag, "connect"))
   {
      if (sonoff)
         revk_raw(NULL, sonoff, 1, havepower ? "1" : "0", 0);   // Ensure power as wanted
      revk_state("power", "%d", havepower);     // Report power state
      revk_state("busy", "%d", busy);   // Report busy state
   }
   if (!strcmp(tag, "restart"))
      wantpower = 0;
   if (!strcmp(tag, "cave"))
      docave = 1;
   if (!strcmp(tag, "on"))
   {
      manual = 1;
      power_needed();
   }
   if (!strcmp(tag, "off"))
   {
      wantpower = 0;
      manual = 0;
   }
   if (!strcmp(tag, "echo"))
      doecho = 1;
   if (!strcmp(tag, "noecho"))
      doecho = 0;
   if (!strcmp(tag, "tx") || !strcmp(tag, "txraw") || !strcmp(tag, "raw"))
   {                            // raw send
      power_needed();
      while (len--)
         queuebyte(*value++);
   }
   if (!strcmp(tag, "text"))
   {                            // Text send
      power_needed();
      while (len--)
      {
         uint32_t b = *value++;
         // We assume this is utf-8
         if ((b & 0xC0) == 0x80)
            b = 0x7F;           // Silly, not even utf-8, rub out
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
            b = 'A' + b - 0x1D670;      // Maths monospace characters
         else if (b >= 0x1D68A && b < 0x1D68A + 26)
            b = 'a' + b - 0x1D68A;
         else if (b >= 0x1D7F6 && b < 0x1D7F6 + 10)
            b = '0' + b - 0x1D7F6;
         else if (b == 0x2014)
            b = '-';            // Common hyphen (long)
         else if (b == 0x2018 || b == 0x2019)
            b = '\'';           // Common quote escape
         else if (b == 0x201c || b == 0x201d)
            b = '"';            // Common quote escape
         if (b >= 0x80)
            b = 0x5E;           // Other unicode so print as Up arrow
         if (b >= ' ' && b < 0x7F && pos >= LINELEN)
            nl();               // Force newline as would overprint
         if (b == '\n')
            nl();               // We want a new line (does CR too)
         else if (b == '\r')
         {                      // Explicit CR, let's assume no NL
            cr();               // We want a carriage return
            queuebyte(0);       // Assuming no LF, need extra null
         } else
            queuebyte(pe(b));
      }
   }
   if (!strcmp(tag, "punch") || !strcmp(tag, "punchraw"))
   {                            // Raw punched data (with DC2/DC4)
      power_needed();
      if (!tag[5])
      {
         if (!nodc4)
            queuebyte(DC2);     // Tape on
         for (int i = 0; i < tapelead; i++)
            queuebyte(0);
      }
      while (len--)
      {
         queuebyte(*value);
         if (!nodc4 && (*value & 0x7F) == DC4)
            queuebyte(DC2);     // Turn tape back on
         value++;
      }
      if (!tag[5])
      {
         for (int i = 0; i < tapetail; i++)
            queuebyte(0);
         if (!nodc4)
            queuebyte(DC4);     // Tape off
      }
   }
   if (!strcmp(tag, "tape") || !strcmp(tag, "taperaw"))
   {                            // Punched tape text
      power_needed();
      if (!tag[4])
      {
         if (!nodc4)
            queuebyte(DC2);     // Tape on
         for (int i = 0; i < tapelead; i++)
            queuebyte(0);
      }
      while (len--)
      {
         int c = *value++;
         const unsigned char *d = small_f[c];
         if (!*d && c >= 'a' && c <= 'z')
            d = small_f[c - 32];        // Try upper case
         int l = 5;
         while (l && !d[l - 1])
            l--;
         if (c == ' ' && l < 3)
            l = 3;
         if (!l)
            continue;
         while (l--)
         {
            queuebyte(*d);
            if (!nodc4 && (*d & 0x7F) == DC4)
               queuebyte(DC2);
            d++;
         }
         if (len)
            queuebyte(0);
      }
      if (!tag[4])
      {
         for (int i = 0; i < tapetail; i++)
            queuebyte(0);
         if (!nodc4)
            queuebyte(DC4);     // Tape off
      }
   }
   return "";
}

void asr33_main(void *param)
{
   queue_mutex = xSemaphoreCreateMutex();
   revk_init(&app_command);
#define u32(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u16(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u8(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u1(n) revk_register(#n,0,sizeof(n),&n,NULL,SETTING_BOOLEAN);
#define t(n) revk_register(#n,0,0,&n,NULL,0);
   settings;
#undef t
#undef u32
#undef u16
#undef u8
#undef u1

   doecho = echo;
   ESP_ERROR_CHECK(uart_driver_install(uart, 1024, 1024, 0, NULL, 0));
   uart_config_t uart_config = {
      .baud_rate = 110,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_2,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
   };
// Configure UART parameters
   uart_param_config(uart, &uart_config);
   uart_set_pin(uart, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
   uart_set_line_inverse(uart, UART_SIGNAL_RXD_INV);
   uart_set_rx_full_threshold(uart, 1);
   gpio_set_pull_mode(rx, GPIO_PULLUP_ONLY);
   gpio_pullup_en(rx);
   if (GPIO_IS_VALID_GPIO(on))
   {                            // On input
      gpio_set_direction(on, GPIO_MODE_INPUT);
      gpio_set_pull_mode(on, GPIO_PULLUP_ONLY);
      gpio_pullup_en(on);
   }
   if (GPIO_IS_VALID_GPIO(pu))
   {                            // Extra pull up
      gpio_set_direction(pu, GPIO_MODE_INPUT);
      gpio_set_pull_mode(pu, GPIO_PULLUP_ONLY);
      gpio_pullup_en(pu);
   }
   if (GPIO_IS_VALID_GPIO(power))
   {
      gpio_set_level(power, 1);
      gpio_set_direction(power, GPIO_MODE_OUTPUT);
   }
   if (GPIO_IS_VALID_GPIO(motor))
   {
      gpio_set_level(motor, 1);
      gpio_set_direction(motor, GPIO_MODE_OUTPUT);
   }

   while (1)
   {
      usleep(10000);
      if (GPIO_IS_VALID_GPIO(on) && gpio_get_level(on))
      {                         // Key press to start/stop
         if (!pressed)
         {                      // Button pressed
            pressed = 1;
            manual = 0;
            if (!wantpower)
               power_needed();
            else
               wantpower = 0;
            revk_event("on", "%d", wantpower);
         }
      } else
         pressed = 0;
      // Check tx buffer usage
      if ((txi + MAXTX - txo) % MAXTX > MAXTX * 2 / 3 && busy != 1)
         revk_state("busy", "%d", busy = 1);
      if ((txi + MAXTX - txo) % MAXTX < MAXTX * 1 / 3 && busy != 0)
         revk_state("busy", "%d", busy = 0);
      if (txi != txo)
         wantpower = 1;
      // Check rx
      size_t len;
      uart_get_buffered_data_len(uart, &len);
      if (len > 0)
      {
         uint8_t b;
         if (uart_read_bytes(uart, &b, 1, 0) > 0)
         {
            if (b && b != 0xFF && b == pe(b))
            {
               if (b == pe(EOT))
                  manual = 0;   // EOT, short timeout
               else
                  manual = 1;   // Typing
            }
            revk_event("rx", "%c", b);
            if ((b & 0x7F) == '\n')
            {
               revk_event("line", "%.*s", rxp, line);
               rxp = 0;
            } else if ((b & 0x7F) != '\r' && rxp < MAXRX)
               line[rxp++] = (b & 0x7F);
            if (doecho)
               queuebyte(b);
            if (b == pe(RU))
               docave = 1;
            if (b == pe(WRU) && (ver || wru))
            {                   // WRU
               // See 3.27 of ISS 8, SECTION 574-122-700TC
               queuebyte(pe('\r'));     // CR
               queuebyte(pe('\n'));     // LF
               queuebyte(pe(0x7F));     // RO
               if (wru)
                  for (const char *p = wru; *p; p++)
                     queuebyte(pe(*p));
               if (ver)
               {
                  for (const char *p = revk_app; *p; p++)
                     queuebyte(pe(*p));
                  for (const char *p = " BUILD "; *p; p++)
                     queuebyte(pe(*p));
                  for (const char *p = revk_version; *p; p++)
                     queuebyte(pe(*p));
               }
               queuebyte(pe('\r'));     // CR
               queuebyte(pe('\n'));     // LF
               queuebyte(pe(ack));      // ACK
            }
         }
      }
      if (wantpower != havepower)
      {                         // Change power state (does any necessary timing sequence)
         if (wantpower == 0)
            power_off();
         else if (wantpower == 1)
            power_on();
      }
      if (sonoff && havepower != 1)
         continue;              // Not running
      int64_t now = esp_timer_get_time();
      // Check tx
      if (txi == txo)
      {                         // Nothing to send
         if (cave || docave)
         {                      // Let's play a game
            docave = 0;
            extern int advent(void);
            if (!busy)
               revk_state("busy", "%d", busy = 1);
            advent();
         }
         if (done < now)
            wantpower = 0;      // Idle complete, power off
         continue;
      }
      if (eot > now + 2000000)
         continue;              // Let's wait for these to send...
      // Get next character
      uint32_t n = txo + 1;
      if (n >= MAXTX)
         n = 0;
      uint8_t b = buf[txo];
      txo = n;
      uart_write_bytes(uart, &b, 1);
      timeout(100000);
   }
}

void app_main(void)
{
   TaskHandle_t task_id = NULL;
   xTaskCreatePinnedToCore(asr33_main, "asr33", 20 * 1024, NULL, 2, &task_id, 1);
}
