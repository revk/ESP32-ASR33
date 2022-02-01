// Simple soft UART for slow speed duplex UART operation, e.g. 110 Baud, with break detect
// Copyright © 2022 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
//
// This works on a timer interrupt at 5 x bit rate
// Start bit accepted after 2 bits low
// Receive sample by middle 3 bits majority

#include "softuart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <driver/timer.h>
#include <driver/gpio.h>

#define	STEPS	5               // Interrupts per clock

struct softuart_s {
   int8_t timer;                // Which timer
   uint16_t baudx100;           // Baud rate, x 100
   uint8_t bits;                // Bits
   int8_t stops;                // Stop bits in interrupts

   int8_t tx;                   // Tx GPIO
   uint8_t txdata[32];          // The tx message
   volatile uint8_t txi;        // Next byte to which new tx byte to be written (set by non int)
   volatile uint8_t txo;        // Next byte from which a tx byte will be read (set by int)
   uint8_t txbit;               // Tx bit count, 0 means idle
   volatile uint8_t txsubbit;   // Tx sub bit count
   uint8_t txbyte;              // Tx byte being clocked in
   volatile uint8_t txbreak;    // Tx break (bit count up to max)

   int8_t rx;                   // Rx pin (can be same as tx)
   uint8_t rxdata[32];          // The Rx data
   volatile uint8_t rxi;        // Next byte to which new rx byte to be written (set by int)
   volatile uint8_t rxo;        // Next byte from which a rx byte will be read (set by non int)
   uint8_t rxbit;               // Rx bit count, 0 means idle
   uint8_t rxsubbit;            // Rx sub bit count
   uint8_t rxbyte;              // Rx byte being clocked in
   volatile uint8_t rxbreak;    // Rx break (bit count up to max)

    uint8_t:0;                  //      Bits set/used from int
   uint8_t txinv;               // Invert tx
   uint8_t rxinv;               // Invert rx
   uint8_t rxlast:1;            // Last rx bit
   uint8_t txnext:1;            // Next tx bit

    uint8_t:0;                  //      Bits set from non int
   uint8_t started:1;           // Int handler started
};

#define TIMER_DIVIDER         4 //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

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
   if (!u->txsubbit)
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
         uint8_t txi = u->txi;
         uint8_t txo = u->txo;
         if (!u->txnext)
         {
            u->txsubbit = u->stops;
            u->txnext = 1;      // End of a break.
         } else if (txi != txo)
         {                      // We have a byte
            u->txbyte = u->txdata[txo];
            txo++;
            if (txo == sizeof(u->txdata))
               txo = 0;
            u->txo = txo;
            u->txbit = u->bits + 1;
            u->txsubbit = STEPS;        // Start bit
            u->txnext = 0;
         } else if (u->txbreak)
         {
            u->txsubbit = u->txbreak;
            u->txbreak = 0;
            u->txnext = 0;
         }
      }
   }
   // Rx
   if (!u->rxbit)
   {                            // Looking for start
      if (!r && !u->rxlast)
      {                         // Start bit (i.e. two samples in a row)

      }
   }
   if (u->rxbit)
   {                            // Clock in data

   }
   u->rxlast = r;

   return false;
}

// Set up
softuart_t *softuart_init(int8_t timer, int8_t tx, uint8_t txinv, int8_t rx, uint8_t rxinv, uint16_t baudx100, uint8_t bits, uint8_t stopx2)
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
   gpio_reset_pin(u->tx);
   gpio_set(u->tx);
   gpio_set_direction(u->tx, GPIO_MODE_OUTPUT);
   gpio_reset_pin(u->rx);
   gpio_set_direction(u->rx, GPIO_MODE_INPUT);
   return u;
}

void softuart_start(softuart_t * u)
{
   if (u->started)
      return;
   u->started = 1;
   // Set up timer
   timer_config_t config;
   memset(&config, 0, sizeof(config));
   config.divider = TIMER_DIVIDER;
   config.counter_dir = TIMER_COUNT_UP;
   config.counter_en = TIMER_PAUSE;
   config.alarm_en = TIMER_ALARM_EN;
   config.intr_type = TIMER_INTR_LEVEL;
   config.auto_reload = 1;
   timer_init(0, u->timer, &config);
   timer_set_counter_value(0, u->timer, 0x00000000ULL);
   timer_set_alarm_value(0, u->timer, (uint64_t) TIMER_SCALE * (100 / STEPS) / (uint64_t) (u->baudx100));
   timer_isr_callback_add(0, u->timer, timer_isr, u, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM);
   timer_enable_intr(0, u->timer);
   timer_start(0, u->timer);
}

void *softuart_end(softuart_t * u)
{
   if (u)
   {
      if (u->started)
         timer_disable_intr(0, u->timer);
      free(u);
   }
   return NULL;
}

// Low level messaging
void softuart_tx(softuart_t * u, uint8_t b)
{
   while (!softuart_tx_space(u))
      usleep(10000);
   uint8_t txi = u->txi;
   u->txdata[txi] = b;
   txi++;
   if (txi == sizeof(u->txdata))
      txi = 0;
   u->txi = txi;
}

uint8_t softuart_rx(softuart_t * u)
{
   while (softuart_rx_ready(u) <= 0)
      usleep(1000);
   uint8_t rxo = u->rxo;
   uint8_t b = u->rxdata[rxo];
   rxo++;
   if (rxo == sizeof(u->rxdata))
      rxo = 0;
   u->rxo = rxo;
   return b;
}

int softuart_tx_space(softuart_t * u)
{                               // Report how much space for sending
   int s = (int) u->txi - (int) u->txo;
   if (s < 0)
      s += sizeof(u->txdata);
   return sizeof(u->txdata) - 1 - s;    // -1 as never completely fills
}

int softuart_tx_waiting(softuart_t * u)
{                               // Report how many bytes still being transmitted including one in process of transmission
   int s = (int) u->txi - (int) u->txo;
   if (s < 0)
      s += sizeof(u->txdata);
   if (u->txsubbit)
      s++;                      // Sending a byte
   return s;
}

void softuart_tx_flush(softuart_t * u)
{                               // Wait for all tx to complete
   while (softuart_tx_waiting(u))
      usleep(1000);
}

void softuart_tx_break(softuart_t * u)
{                               // Send a break (once tx done)
   u->txbreak = 255;
}

int softuart_rx_ready(softuart_t * u)
{                               // Report how many bytes are available to read (negative means BREAK)
   int s = (int) u->rxi - (int) u->rxo;
   if (s < 0)
      s += sizeof(u->rxdata);
   if (!s)
      s = -(int) u->rxbreak;
   return s;
}
