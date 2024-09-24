// Simple soft UART for slow speed duplex UART operation, e.g. 110 Baud, with break detect
// Copyright © 2022 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0

#ifndef	SOFTUART_H
#define	SOFTUART_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

typedef struct softuart_s softuart_t;
typedef struct softuart_stats_s softuart_stats_t;
struct softuart_stats_s
{
   uint32_t tx;
   uint32_t rx;
   uint32_t rxbadstart;
   uint32_t rxbadstop;
   uint32_t rxbad0;
   uint32_t rxbad1;
   uint32_t rxbadp;
};

// Set up
softuart_t *softuart_init (int8_t timer, revk_gpio_t tx, revk_gpio_t rx, uint16_t baudx100, uint8_t bits, uint8_t stopx2,
                           uint8_t linelen, uint16_t crm);
void softuart_start (softuart_t *);
void *softuart_end (softuart_t *);

void softuart_stats (softuart_t *, softuart_stats_t *); // Get (and clear) stats
int softuart_tx_space (softuart_t *);   // Report how much space for sending
int softuart_tx_waiting (softuart_t *); // Report how many bytes still being transmitted including one in process of transmission
void softuart_tx_flush (softuart_t *);  // Wait for all tx to complete
void softuart_tx (softuart_t *, uint8_t b);     // Send byte, blocking
void softuart_tx_break (softuart_t *, uint8_t chars);   // Send a break (number of chars)
void softuart_xoff (softuart_t *);      // Stop sending
void softuart_xon (softuart_t *);       // Start sending
int softuart_rx_ready (softuart_t *);   // Report how many bytes are available to read (-1 means BREAK)
uint8_t softuart_rx (softuart_t *);     // Receive byte, blocking
uint8_t pe (uint8_t);           // Parity (even)

#endif
