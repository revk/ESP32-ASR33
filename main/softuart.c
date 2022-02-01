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
   uint8_t txbit;               // Tx bit count, 0 means idle
   uint8_t txsubbit;            // Tx sub bit count

   int8_t rx;                   // Rx pin (can be same as tx)
   uint8_t rxdata[32];          // The Rx data
   uint8_t rxbit;               // Rx bit count, 0 means idle
   uint8_t rxsubbit;            // Rx sub bit count

    uint8_t:0;                  //      Bits set/used from int
   uint8_t rlast:1;             // Last rx bit

    uint8_t:0;                  //      Bits set from non int
   uint8_t started:1;           // Int handler started
};

#define TIMER_DIVIDER         4 //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

// Low level direct GPIO controls - inlines were not playing with some optimisation modes
#define gpio_set(r) do{if ((r) >= 32)GPIO_REG_WRITE(GPIO_OUT1_W1TS_REG, 1 << ((r) - 32)); else if ((r) >= 0)GPIO_REG_WRITE(GPIO_OUT_W1TS_REG, 1 << (r));}while(0)
#define gpio_clr(r) do{if ((r) >= 32)GPIO_REG_WRITE(GPIO_OUT1_W1TC_REG, 1 << ((r) - 32));else if ((r) >= 0)GPIO_REG_WRITE(GPIO_OUT_W1TC_REG, 1 << (r));}while(0)
#define gpio_get(r) (((r) >= 32)?((GPIO_REG_READ(GPIO_IN1_REG) >> ((r) - 32)) & 1):((r) >= 0)?((GPIO_REG_READ(GPIO_IN_REG) >> (r)) & 1):0)

bool IRAM_ATTR timer_isr(void *gp)
{
   softuart_t *g = gp;
   // Sample rx
   uint8_t r = gpio_get(g->rx);
   // Work out next tx
   if (!g->txbit)
   {                            // Do we have a next byte to start

   }
   if (g->txbit)
   {                            // Sending next bit

   }
   uint8_t t = 1;
   // Set tx
   if (t)
      gpio_set(g->tx);
   else
      gpio_clr(g->tx);
   // Process rx
   if (!g->rxbit)
   {                            // Looking for start
      if (!r && !g->rlast)
      {                         // Start bit (i.e. two samples in a row)

      }
   }
   if (g->rxbit)
   {                            // Clock in data

   }
   g->rlast = r;

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
   softuart_t *g = malloc(sizeof(*g));
   if (!g)
      return g;
   memset(g, 0, sizeof(*g));
   g->baudx100 = (baudx100 ? : 11000);
   g->bits = (bits ? : 8);
   g->stops = ((stopx2 ? : 4) * STEPS + 1) / 2;
   g->tx = tx;
   g->rx = rx;
   g->timer = timer;
   gpio_reset_pin(g->tx);
   gpio_set(g->tx);
   gpio_set_direction(g->tx, GPIO_MODE_OUTPUT);
   gpio_reset_pin(g->rx);
   gpio_set_direction(g->rx, GPIO_MODE_INPUT);
   return g;
}

void softuart_start(softuart_t * g)
{
   if (g->started)
      return;
   g->started = 1;
   // Set up timer
   timer_config_t config;
   memset(&config, 0, sizeof(config));
   config.divider = TIMER_DIVIDER;
   config.counter_dir = TIMER_COUNT_UP;
   config.counter_en = TIMER_PAUSE;
   config.alarm_en = TIMER_ALARM_EN;
   config.intr_type = TIMER_INTR_LEVEL;
   config.auto_reload = 1;
   timer_init(0, g->timer, &config);
   timer_set_counter_value(0, g->timer, 0x00000000ULL);
   timer_set_alarm_value(0, g->timer, (uint64_t) TIMER_SCALE * (100 / STEPS) / (uint64_t) (g->baudx100));
   timer_isr_callback_add(0, g->timer, timer_isr, g, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM);
   timer_enable_intr(0, g->timer);
   timer_start(0, g->timer);
}

void *softuart_end(softuart_t * g)
{
   if (g)
   {
      if (g->started)
         timer_disable_intr(0, g->timer);
      free(g);
   }
   return NULL;
}

// Low level messaging
int softuart_tx(softuart_t * g, int len, uint8_t * data)
{

   return 0;
}

int softuart_rx(softuart_t * g, int max, uint8_t * data)
{

   return 0;
}
