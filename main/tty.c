// Functions to talk to the TTY
// Expects globals from ASR33.c
// This is a wrapper, was for hard and soft UART but now only for soft UART

#include <driver/gpio.h>
#include "revk.h"
#include "softuart.h"
#include "tty.h"

static softuart_t *u = NULL;

void
tty_setup (void)
{                               // Does UART setup, expects uart to be set globally, UART number for hard, or negative for soft
   u = softuart_init (0, tx, rx, baud, databits, stop / 5, linelen, timecr);
   if (!u)
      ESP_LOGE ("TTY", "Failed to init soft uart");
   else
      softuart_start (u);
}

void
tty_flush (void)
{                               // Waits for all Tx to complete and returns
   softuart_tx_flush (u);
   return;
}

int
tty_rx_ready (void)
{                               // Reports how many bytes ready to read, 0 if none, negative for BREAK condition (after all Rx read)
   return softuart_rx_ready (u);
}

void
tty_break (uint8_t chars)
{                               // Send a break (softuart)
   softuart_tx_break (u, chars);
}

void
tty_tx (uint8_t b)
{                               // Send a byte, blocking
   softuart_tx (u, b);
}

uint8_t
tty_rx (void)
{                               // Receive a byte, blocking
   return softuart_rx (u);      // Soft UART
}

int
tty_tx_space (void)
{
   return softuart_tx_space (u);        // Soft UART
}

int
tty_tx_waiting (void)
{
   return softuart_tx_waiting (u);
}

void
tty_xoff (void)
{
   softuart_xoff (u);
}

void
tty_xon (void)
{
   softuart_xon (u);
}

void
tty_stats (softuart_stats_t * s)
{
   softuart_stats (u, s);
}
