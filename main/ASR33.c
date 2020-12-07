// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
static const char TAG[] = "ASR33";

#include "revk.h"
#include <esp_spi_flash.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#define settings  \
  u8(uart,1);	\
  u8(tx,17);	\
  u8(rx,16);	\
  u8(pu,0xFF);	\
  u8(on,4);	\
  u1(echo);	\
  t(sonoff);	\
  u32(wake,1)	\
  u32(idle,10)	\
  u32(keyidle,120)	\

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
#define	MAXTX	65536
#define	MAXRX	256

volatile int8_t manual = 0;     // Manual power override (eg key pressed, etc)
int8_t busy = 0;                // Busy state
volatile int8_t power = -1;     // Power state
uint8_t buf[MAXTX];             // Tx pending buffer
volatile uint32_t txi = 0;      // Tx buffer in pointer
volatile uint32_t txo = 0;      // Tx buffer out pointer
uint8_t line[MAXRX];            // Rx line buffer
uint32_t rxp = 0;               // Rx line buffer pointer
int64_t eot = 0;                // When tx expected to end
volatile int64_t done = 0;      // When to next turn off
volatile int64_t start = 0;     // When to start printing
uint8_t pos = 0;                // Position (ignores local echo)

uint8_t pe(uint8_t b)
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

void txbyte(uint8_t b)
{
   uart_write_bytes(uart, &b, 1);
   timeout(100000);
}

void queuebyte(uint8_t b)
{                               // Queue a byte
   uint32_t n = txi + 1;
   if (n >= MAXTX)
      n = 0;
   if (n == txo)
      return;                   // Too much
   buf[txi] = b;
   txi = n;
   b &= 0x7F;
   if (b == '\r')
      pos = 0;
   else if (b >= ' ' && b < 0x7F && pos < 72)
      pos++;
}

void power_off(void)
{
   if (power == 0)
      return;
   revk_state("power", "%d", power = 0);
   if (sonoff)
      revk_raw(NULL, sonoff, 1, "0", 0);
   manual = 0;
   start = 0;
   done = 0;
   txi = 0;
   txo = 0;
   rxp = 0;
}

void power_on(void)
{
   if (power == 1)
      return;
   if (sonoff)
      revk_raw(NULL, sonoff, 1, "1", 0);
   start = timeout(0) + wake * 1000;    // Delayed start for power on
   revk_state("power", "%d", power = 1);
}

const char *app_command(const char *tag, unsigned int len, const unsigned char *value)
{
   if (!strcmp(tag, "connect"))
   {
      revk_info(TAG, "Running ASR33 control");
      if (power < 0)
         power_off();
      else
         revk_state("power", "%d", power);
      revk_state("busy", "%d", busy);
   }
   if (!strcmp(tag, "restart"))
      power_off();
   if (!strcmp(tag, "on"))
   {
      manual = 1;
      power_on();
   }
   if (!strcmp(tag, "off"))
   {
      power_off();
      manual = 0;
   }
   if (!strcmp(tag, "tx"))
   {                            // raw send
      power_on();
      while (len--)
         queuebyte(*value++);
   }
   if (!strcmp(tag, "text"))
   {                            // Text send
      power_on();
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
         if (b >= 0x80)
            b = 0x5E;           // Other unicode so print as Up arrow
         if (b >= ' ' && b < 0x7F && pos >= 72)
         {                      // force newline before printing beyond the end
            queuebyte(pe('\r'));        // CR before LF for timing
            queuebyte(0);
            queuebyte(pe('\n'));
         } else if (b == '\n')
         {
            queuebyte(pe('\r'));        // Add CR before LF
            queuebyte(0);
         }
         queuebyte(pe(b));
      }
   }
   return "";
}

void app_main()
{
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
   {
      gpio_set_pull_mode(pu, GPIO_PULLUP_ONLY);
      gpio_pullup_en(pu);
   }

   while (1)
   {
      usleep(10000);
      if (GPIO_IS_VALID_GPIO(on) && gpio_get_level(on) && !power)
      {                         // Key press to start
         manual = 0;
         power_on();
         revk_event("on", "1");
      }
      // Check tx buffer usage
      if ((txi + MAXTX - txo) % MAXTX > MAXTX * 2 / 3 && busy != 1)
         revk_state("busy", "%d", busy = 1);
      if ((txi + MAXTX - txo) % MAXTX < MAXTX * 1 / 3 && busy != 0)
         revk_state("busy", "%d", busy = 0);
      if (power != 1)
         continue;              // Not running
      int64_t now = esp_timer_get_time();
      if (start > now)
         continue;              // Waiting to start
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
               if ((b & 0x7F) == 4)
                  manual = 0;   // EOT, short timeout
               else
                  manual = 1;   // Typing
            }
            timeout(0);
            revk_event("rx", "%c", b);
            if (b == '\n')
            {
               revk_event("line", "%.*s", rxp, line);
               rxp = 0;
            } else if (b != '\r' && rxp < MAXRX)
               line[rxp++] = (b & 0x7F);
            if (echo)
               txbyte(b);
         }
      }
      // Check tx
      if (txi == txo)
      {                         // Nothing to send
         if (done < now)
            power_off();        // Idle complete, power off
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
      txbyte(b);
   }
}
