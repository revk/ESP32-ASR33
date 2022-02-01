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
softuart_t *softuart_init(int8_t timer, int8_t tx, int8_t rx, uint16_t baudx100, uint8_t bits, uint8_t stopx2);
void softuart_start(softuart_t *);
void *softuart_end(softuart_t *);

int softuart_tx_space(softuart_t *);    // Report how much space for sending
int softuart_tx_waiting(softuart_t *);  // Report how many bytes still being transmitted including one in process of transmission
void softuart_tx_flush(softuart_t *);   // Wait for all tx to complete
int softuart_tx(softuart_t *, int len, uint8_t * buf);  // Send bytes (return len sent, or -ve for error)
int softuart_rx_ready(softuart_t *);    // Report how many bytes are available to read (-1 means BREAK)
int softuart_rx(softuart_t *, int max, uint8_t * buf);  // Receive bytes (return len received, or -ve for error)

#endif
