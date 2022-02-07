// Simple soft UART for slow speed duplex UART operation, e.g. 110 Baud, with break detect
// Copyright © 2022 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
//
// This works on a timer interrupt at 5 x bit rate
// Start bit accepted after 2 samples low
// Receive sample by middle 3 samples majority in each bit
// Would be nice some time to have some stats for bad start bits, and bad quality bits (e.g. 1 or 2 counts not 0 or 3)

#include "softuart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <driver/timer.h>
#include <driver/gpio.h>

#define	STEPS	5               // Interrupts per clock

struct softuart_s {
   SemaphoreHandle_t mutex;     // Protect softuart_tx

   int8_t timer;                // Which timer
   uint16_t baudx100;           // Baud rate, x 100
   uint8_t bits;                // Bits
   int8_t stops;                // Stop bits in interrupts

   int8_t tx;                   // Tx GPIO
   uint8_t txdata[32768];       // The tx message
   volatile uint16_t txi;       // Next byte to which new tx byte to be written (set by non int)
   volatile uint16_t txo;       // Next byte from which a tx byte will be read (set by int)
   uint8_t txbit;               // Tx bit count, 0 means idle
   volatile uint8_t txsubbit;   // Tx sub bit count
   uint8_t txbyte;              // Tx byte being clocked in
   volatile uint8_t txbreak;    // Tx break (bit count up to max)
   uint8_t crwait;              // Tx waiting for CR (sub bit count down)
   uint8_t crline;              // Tx wait extra sub bits for whole line
   uint8_t linelen;             // Line len
   uint8_t pos;                 // Carriage posn

   int8_t rx;                   // Rx pin (can be same as tx)
   uint8_t rxdata[32];          // The Rx data
   volatile uint8_t rxi;        // Next byte to which new rx byte to be written (set by int)
   volatile uint8_t rxo;        // Next byte from which a rx byte will be read (set by non int)
   uint8_t rxbit;               // Rx bit count, 0 means idle
   uint8_t rxsubbit;            // Rx sub bit count
   uint8_t rxcount;             // Rx sub bit 1 count for majority check in mid bit
   uint8_t rxbyte;              // Rx byte being clocked in
   volatile uint16_t rxbreak;   // Rx break (bit count up to max)

    uint8_t:0;                  //      Bits set from int
   uint8_t txinv;               // Invert tx
   uint8_t rxinv;               // Invert rx
   uint8_t rxlast:1;            // Last rx bit
   uint8_t txnext:1;            // Next tx bit

    uint8_t:0;                  //      Bits set from non int
   uint8_t started:1;           // Int handler started
   uint8_t txwait:1;            // Hold off on tx
   softuart_stats_t stats;
};

// Low level direct GPIO controls - inlines were not playing with some optimisation modes
#define gpio_set(r) do{if ((r) >= 32)GPIO_REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << ((r) - 32)); else if ((r) >= 0)GPIO_REG_WRITE(GPIO_OUT_W1TS_REG, 1 << (r));}while(0)
#define gpio_clr(r) do{if ((r) >= 32)GPIO_REG_WRITE(GPIO_OUT1_W1TC_REG, 1 << ((r) - 32));else if ((r) >= 0)GPIO_REG_WRITE(GPIO_OUT_W1TC_REG, 1 << (r));}while(0)
#define gpio_get(r) (((r) >= 32)?((GPIO_REG_READ(GPIO_IN1_REG) >> ((r) - 32)) & 1):((r) >= 0)?((GPIO_REG_READ(GPIO_IN_REG) >> (r)) & 1):0)

bool IRAM_ATTR timer_isr(void *up)
{
   softuart_t *u = up;
   // Timing based, sample Rx and set Tx
   uint8_t r = (gpio_get(u->rx) ^ u->rxinv);
   if (u->txnext ^ u->txinv)
      gpio_set(u->tx);
   else
      gpio_clr(u->tx);
   // Tx
   if (u->txsubbit)
      u->txsubbit--;            // Working through sub bits
   if (u->crwait)
      u->crwait--;
   if (!u->txsubbit && !u->txwait)
   {                            // Work out next tx
      if (u->txbit)
      {                         // Sending a byte
         u->txbit--;
         if (!u->txbit)
         {                      // Stop bits at end of byte
            u->txsubbit = u->stops;
            u->txnext = 1;
         } else
         {
            u->txsubbit = STEPS;        // Send next bit
            u->txnext = (u->txbyte & 1);
            u->txbyte >>= 1;
         }
      } else if (!u->txbit)
      {                         // Do we have a next byte to start
         uint16_t txi = u->txi;
         uint16_t txo = u->txo;
         if (!u->txnext)
         {
            u->txsubbit = u->stops;
            u->txnext = 1;      // End of a break.
         } else if (txi != txo)
         {                      // We have a byte
            u->txbyte = u->txdata[txo];
            uint8_t b = (u->txbyte & 0x7F);
            if (!u->crwait || b < ' ' || b >= 0x7F)
            {                   // Either Ok to send not (CR time done) or non printable, so OK to send anyway
               txo++;
               if (txo == sizeof(u->txdata))
                  txo = 0;
               u->txo = txo;
               u->txbit = u->bits + 1;
               u->txsubbit = STEPS;     // Start bit
               u->txnext = 0;
               if (b == '\r')
               {                // CR
                  if (!u->crwait)
                     u->crwait = (int) u->pos * u->crline / u->linelen + (1 + u->bits) * STEPS + u->stops;      // Allow extra time for CR
                  u->pos = 0;
               } else if (b >= ' ' && b < 0x7F && u->pos < u->linelen)
                  u->pos++;
               u->stats.tx++;
            }

         } else if (u->txbreak)
         {
            u->txsubbit = u->txbreak;
            u->txbreak = 0;
            u->txnext = 0;
         }
      }
   }
   // Rx
   if (!u->rxsubbit)
   {                            // Idle, waiting for start bit
      if (!r && !u->rxlast)
      {                         // Start bit
         u->rxsubbit = STEPS - 1;
         u->rxbit = u->bits + 1;        // Data and start
         u->rxbyte = 0;
      }
   }
   if (u->rxsubbit)
   {                            // Clocking in bits, or break
      u->rxsubbit--;
      if (!u->rxsubbit)
      {                         // Bit received
         if (u->rxbit)
         {                      // Clocking in a byte
            if (u->rxbit == u->bits + 1 && u->rxcount >= (STEPS - 1) / 2)
            {
               u->rxbit = 0;    // Bad start bit
               u->stats.rxbadstart++;
            } else
            {
               u->rxbyte >>= 1;
               if (u->rxcount >= (STEPS - 1) / 2)
               {
                  u->rxbyte |= (1 << (u->bits - 1));
                  if (u->rxcount < (STEPS - 2))
                     u->stats.rxbad1++;
               } else if (u->rxcount)
                  u->stats.rxbad0++;
               u->rxbit--;
               u->rxsubbit = STEPS;     // Next bit
            }
         } else
         {                      // Stop bit
            if (!u->rxbreak)
            {                   // Normal (stop bit now clocked in)
               if (u->rxcount < (STEPS - 1) / 2)
               {                // Bad stop bit, don't clock in byte
                  if (!r)
                  {             // Looks like break
                     u->rxbreak = (u->bits + 2) * STEPS;        // Start break condition (we have had this many sub bits)
                     u->rxsubbit = 1;   // Wait end of break
                  }
                  u->stats.rxbadstop++;
                  // else leave rxsubbit unset so we wait for next start bit - don't clock in duff byte
               } else
               {                // Normal end of byte - record received byte (clean start and stop bit)
                  uint8_t rxi = u->rxi;
                  u->rxdata[rxi] = u->rxbyte;
                  rxi++;
                  if (rxi == sizeof(u->rxdata))
                     rxi = 0;
                  if (rxi != u->rxo)
                     u->rxi = rxi;      // Has space
                  u->stats.rx++;
                  // leave rxsubbit unset so we wait for next start bit
               }
            } else
            {                   // Check still in break condition.
               if (!r)
               {                // Still in break
                  if (u->rxbreak < 255)
                     u->rxbreak++;
                  u->rxsubbit = 1;      // Wait end of break
               } else
                  u->rxbreak = 0;       // End of break - leave rxsubbit unset so we wait for next start bit
            }
         }
         u->rxcount = 0;
      } else if (r && u->rxsubbit <= (STEPS - 2))
         u->rxcount++;
   }
   u->rxlast = r;
   return false;
}

   // Set up
softuart_t *softuart_init(int8_t timer, int8_t tx, uint8_t txinv, int8_t rx, uint8_t rxinv, uint16_t baudx100, uint8_t bits, uint8_t stopx2, uint8_t linelen, uint16_t crms)
{
   if (timer < 0 || tx < 0 || rx < 0 || tx == rx ||     //
       !GPIO_IS_VALID_OUTPUT_GPIO(tx)   //
       || !GPIO_IS_VALID_GPIO(rx)       //
       )
      return NULL;
   softuart_t *u = malloc(sizeof(*u));
   if (!u)
      return u;
   memset(u, 0, sizeof(*u));
   u->mutex = xSemaphoreCreateMutex();
   u->baudx100 = (baudx100 ? : 11000);
   u->bits = (bits ? : 8);
   u->stops = ((stopx2 ? : 4) * STEPS + 1) / 2;
   u->tx = tx;
   u->txinv = txinv;
   u->txnext = 1;
   u->rx = rx;
   u->rxinv = rxinv;
   u->rxlast = 1;
   u->timer = timer;
   u->linelen = linelen;
   u->pos = linelen;
   if (crms)
      u->crline = (uint32_t) crms *STEPS * baudx100 / 100000;
   gpio_reset_pin(u->tx);
   gpio_set(u->tx);
   gpio_set_direction(u->tx, GPIO_MODE_OUTPUT);
   gpio_reset_pin(u->rx);
   gpio_set_direction(u->rx, GPIO_MODE_INPUT);
   return u;
}

void softuart_start(softuart_t * u)
{
	if(!u)return;
   if (u->started)
      return;
   u->started = 1;
   uint32_t divider = 2;        // min 2
   uint32_t ticks = (uint64_t) TIMER_BASE_CLK * 100 / STEPS / divider / u->baudx100;
   //ESP_LOGE("UART", "Baudx100=%u Base=%u divider=%d ticks=%u", u->baudx100, TIMER_BASE_CLK, divider, ticks);

   // Set up timer
   timer_config_t config;
   memset(&config, 0, sizeof(config));
   config.divider = divider;
   config.counter_dir = TIMER_COUNT_UP;
   config.counter_en = TIMER_PAUSE;
   config.alarm_en = TIMER_ALARM_EN;
   config.intr_type = TIMER_INTR_LEVEL;
   config.auto_reload = 1;
   timer_init(0, u->timer, &config);
   timer_set_counter_value(0, u->timer, 0x00000000ULL);
   timer_set_alarm_value(0, u->timer, ticks);
   timer_isr_callback_add(0, u->timer, timer_isr, u, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM);
   timer_enable_intr(0, u->timer);
   timer_start(0, u->timer);
}

void *softuart_end(softuart_t * u)
{
	if(!u)return NULL;
      if (u->started)
         timer_disable_intr(0, u->timer);
      if (u->mutex)
         vSemaphoreDelete(u->mutex);
      free(u);
   return NULL;
}

// Low level messaging
void softuart_tx(softuart_t * u, uint8_t b)
{
	if(!u)return;
   xSemaphoreTake(u->mutex, portMAX_DELAY);     // Just to protect from itself, e.g. called from different tasks
   while (!softuart_tx_space(u))
      usleep(10000);
   uint16_t txi = u->txi;
   u->txdata[txi] = b;
   txi++;
   if (txi == sizeof(u->txdata))
      txi = 0;
   u->txi = txi;
   xSemaphoreGive(u->mutex);
}

uint8_t softuart_rx(softuart_t * u)
{
	if(!u)return 0;
   xSemaphoreTake(u->mutex, portMAX_DELAY);     // Just to protect from itself, e.g. called from different tasks
   while (softuart_rx_ready(u) <= 0)
      usleep(1000);
   uint8_t rxo = u->rxo;
   uint8_t b = u->rxdata[rxo];
   rxo++;
   if (rxo == sizeof(u->rxdata))
      rxo = 0;
   u->rxo = rxo;
   xSemaphoreGive(u->mutex);
   return b;
}

int softuart_tx_space(softuart_t * u)
{                               // Report how much space for sending
	if(!u)return 0;
   int s = (int) u->txi - (int) u->txo;
   if (s < 0)
      s += sizeof(u->txdata);
   return sizeof(u->txdata) - 1 - s;    // -1 as never completely fills
}

int softuart_tx_waiting(softuart_t * u)
{                               // Report how many bytes still being transmitted including one in process of transmission
	if(!u)return 0;
   int s = (int) u->txi - (int) u->txo;
   if (s < 0)
      s += sizeof(u->txdata);
   if (u->txsubbit)
      s++;                      // Sending a byte
   return s;
}

void softuart_tx_flush(softuart_t * u)
{                               // Wait for all tx to complete
	if(!u)return;
   while (softuart_tx_waiting(u))
      usleep(1000);
}

void softuart_tx_break(softuart_t * u)
{                               // Send a break (once tx done)
	if(!u)return;
   u->txbreak = 255;
}

int softuart_rx_ready(softuart_t * u)
{                               // Report how many bytes are available to read (negative means BREAK)
	if(!u)return 0;
   int s = (int) u->rxi - (int) u->rxo;
   if (s < 0)
      s += sizeof(u->rxdata);
   if (!s)
      s = -(u->rxbreak / STEPS);        // How many bits of break
   return s;
}

void softuart_xoff(softuart_t * u)
{
	if(!u)return;
   u->txwait = 1;
}

void softuart_xon(softuart_t * u)
{
	if(!u)return;
   u->txwait = 0;
}

void softuart_stats(softuart_t * u, softuart_stats_t * s)
{                               // Get (and clear) stats
	if(!u)return;
   *s = u->stats;
   memset(&u->stats, 0, sizeof(u->stats));
}
