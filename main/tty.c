// Functions to talk to the TTY
// Expects globals from ASR33.c
// This is a wrapper, was for hard and soft UART but now only for soft UART

#include <driver/gpio.h>
#include "revk.h"
#include "softuart.h"
#include "tty.h"
extern uint8_t rx;
extern uint8_t rxpu;
extern uint8_t tx;
extern uint8_t txpu;
extern uint8_t txod;
extern uint16_t baudx100;
extern uint8_t databits;
extern uint8_t stopx2;
extern uint8_t linelen;
extern uint16_t crms;

static softuart_t *u = NULL;

void tty_setup(void)
{                               // Does UART setup, expects uart to be set globally, UART number for hard, or negative for soft
   u = softuart_init(0, port_mask(tx), port_inv(tx), port_mask(rx), port_inv(rx), baudx100, databits, stopx2, linelen, crms);
   if (!u)
      ESP_LOGE("TTY", "Failed to init soft uart");
   else
      softuart_start(u);

   if (rx)
   {
      if (rxpu)
         gpio_pullup_en(port_mask(rx));
      else
         gpio_pullup_dis(port_mask(rx));
   }
   if (tx)
   {
      if (txod)
         gpio_set_direction(tx, GPIO_MODE_DEF_OD);
      if (txpu)
         gpio_pullup_en(port_mask(tx));
      else
         gpio_pullup_dis(port_mask(tx));
   }
}

void tty_flush(void)
{                               // Waits for all Tx to complete and returns
   softuart_tx_flush(u);
   return;
}

int tty_rx_ready(void)
{                               // Reports how many bytes ready to read, 0 if none, negative for BREAK condition (after all Rx read)
   return softuart_rx_ready(u);
}

void tty_break(uint8_t chars)
{                               // Send a break (softuart)
   softuart_tx_break(u, chars);
}

void tty_tx(uint8_t b)
{                               // Send a byte, blocking
   softuart_tx(u, b);
}

uint8_t tty_rx(void)
{                               // Receive a byte, blocking
   return softuart_rx(u);       // Soft UART
}

int tty_tx_space(void)
{
   return softuart_tx_space(u); // Soft UART
}

int tty_tx_waiting(void)
{
   return softuart_tx_waiting(u);
}

void tty_xoff(void)
{
   softuart_xoff(u);
}

void tty_xon(void)
{
   softuart_xon(u);
}

void tty_stats(softuart_stats_t * s)
{
   softuart_stats(u, s);
}
