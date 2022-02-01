// Simple soft UART for slow speed duplex UART operation, e.g. 110 Baud, with break detect
// Copyright © 2022 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
//
// This works on a timer interrupt at 5 x bit rate
// Start bit accepted after 2 bits low
// Receive sample by middle 3 bits majority

// TODO not finished

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
   uint8_t txbit;               // Tx bit count, 0 means idle
   uint8_t txsubbit;            // Tx sub bit count
   uint8_t txbyte;              // Tx byte being clocked in

   int8_t rx;                   // Rx pin (can be same as tx)
   uint8_t rxdata[32];          // The Rx data
   uint8_t rxbit;               // Rx bit count, 0 means idle
   uint8_t rxsubbit;            // Rx sub bit count
   uint8_t rxbyte;              // Tx byte being clocked in

    uint8_t:0;                  //      Bits set/used from int
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
   uint8_t r = gpio_get(u->rx);
   if (u->txnext)
      gpio_set(u->tx);
   else
      gpio_clr(u->tx);
   // Work out next tx
   if (!u->txbit)
   {                            // Do we have a next byte to start

   }
   if (u->txbit)
   {                            // Sending next bit

   }
   // Process rx
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
softuart_t *softuart_init(int8_t timer, int8_t tx, int8_t rx, uint16_t baudx100, uint8_t bits, uint8_t stopx2)
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
   u->rx = rx;
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
int softuart_tx(softuart_t * u, int len, uint8_t * data)
{

   return -1;
}

int softuart_rx(softuart_t * u, int max, uint8_t * data)
{

   return -1;
}

int softuart_tx_space(softuart_t * u)
{                               // Report how much space for sending

   return -1;
}

int softuart_tx_waiting(softuart_t * u)
{                               // Report how many bytes still being transmitted including one in process of transmission
   // -1 means non ready and receiving BREAK condition

   return -1;
}

void softuart_tx_flush(softuart_t * u)
{                               // Wait for all tx to complete

}

int softuart_rx_ready(softuart_t * u)
{                               // Report how many bytes are available to read (-1 means BREAK)

   return -1;
}
