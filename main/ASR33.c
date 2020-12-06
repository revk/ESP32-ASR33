// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
static const char TAG[] = "ASR33";

#include "revk.h"
#include <esp_spi_flash.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#define settings  \
  u8(uart,1);	\
  u8(tx,1);	\
  u8(rx,3);	\
  u1(echo);	\
  t(sonoff);	\
  u32(idle,10)	\

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

int8_t busy = -1;               // Busy state
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

void txbyte(uint8_t b)
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
   int64_t now = esp_timer_get_time();
   start = now + 1000000;
   done = now + 1000000 * idle;
   txbyte(0);
   txbyte(0);
   txbyte(pe('\r'));
   revk_state("power", "%d", power = 1);
}

const char *app_command(const char *tag, unsigned int len, const unsigned char *value)
{
   if (!strcmp(tag, "connect"))
   {
      revk_info(TAG, "Running ASR33 control");
      if (power < 0)
         power_off();
   }
   if (!strcmp(tag, "upgrade"))
      power_off();
   if (!strcmp(tag, "on"))
      power_on();
   if (!strcmp(tag, "off"))
      power_off();
   if (!strcmp(tag, "tx"))
   {                            // raw send
      power_on();
      while (len--)
         txbyte(*value++);
   }
   if (!strcmp(tag, "text"))
   {                            // Text send
      power_on();
      while (len--)
      {
         uint8_t b = *value++;
         if (b >= 0xC0)
            b = 0x5F;           // Unicode
         else if (b > 0x80)
            continue;           // Unicode continue
         txbyte(pe(b));
         if (b == '\n')
            txbyte(pe('\r'));
         if (pos >= 72)
         {
            txbyte(pe('\r'));
            txbyte(pe('\n'));
         }
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
   gpio_set_pull_mode(rx, GPIO_PULLUP_ONLY);

   while (1)
   {
      usleep(50000);
      // Check rx
      size_t len;
      uart_get_buffered_data_len(uart, &len);
      if (len > 0)
      {
         if (!power)
         {
            power_on();
            sleep(1);
         }
         uint8_t b;
         if (uart_read_bytes(uart, &b, 1, 0) > 0)
         {
            revk_event("rx", "%c", b);
            if (b == '\n')
            {
               revk_event("line", "%.*s", rxp, line);
            } else if (rxp < MAXRX)
               line[rxp++] = (b & 0x7F);
            if (echo)
               uart_write_bytes(uart, &b, 1);
         }
      }
      // Check buffer
      if ((txi + MAXTX - txo) % MAXTX > MAXTX * 2 / 3 && busy == 1)
         revk_state("busy", "%d", busy = 1);
      if ((txi + MAXTX - txo) % MAXTX < MAXTX * 1 / 3 && busy == 0)
         revk_state("busy", "%d", busy = 0);
      if (power != 1)
         continue;              // Not running
      int64_t now = esp_timer_get_time();
      if (start > now)
         continue;              // Waiting to start
      if (txi == txo)
      {
         if (done < now)
            power_off();
         continue;              // Nothing to send
      }
      if (eot > now + 1000000)
         continue;              // Waiting to buffer more
      uint32_t n = txo + 1;
      if (n >= MAXTX)
         n = 0;
      uint8_t b = buf[txo];
      txo = n;
      uart_write_bytes(uart, &b, 1);
      if (eot < now)
         eot = now;
      eot += 100000;
      done = eot + 1000000 * idle;
   }
}
