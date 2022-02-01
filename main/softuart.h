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

// Set up
softuart_t *softuart_init(int8_t timer, int8_t tx, uint8_t txinv, int8_t rx, uint8_t rxinv, uint16_t baudx100, uint8_t bits, uint8_t stopx2);
void softuart_start(softuart_t *);
void *softuart_end(softuart_t *);

int softuart_tx_space(softuart_t *);    // Report how much space for sending
int softuart_tx_waiting(softuart_t *);  // Report how many bytes still being transmitted including one in process of transmission
void softuart_tx_flush(softuart_t *);   // Wait for all tx to complete
void softuart_tx(softuart_t *, uint8_t b);      // Send byte, blocking
void softuart_tx_break(softuart_t *);   // Send a break
int softuart_rx_ready(softuart_t *);    // Report how many bytes are available to read (-1 means BREAK)
uint8_t softuart_rx(softuart_t *);      // Receive byte, blocking

#endif