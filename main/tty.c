// Functions to talk to the TTY
// Expects globals from ASR33.c

#include <driver/gpio.h>
#include <driver/uart.h>
#include "tty.h"
extern int8_t uart;
extern uint8_t rx;
extern uint8_t tx;
extern uint16_t baud;
extern uint8_t databits;
extern uint8_t halfstops;

void tty_setup(void)
{                               // Does UART setup, expects uart to be set globally, UART number for hard, or negative for soft
   if (uart < 0)
   {                            // Soft UART
      // TODO
      return;
   }
   // Hard UART
   uart_driver_install(uart, 1024, 1024, 0, NULL, 0);
   uart_config_t uart_config = {
      .baud_rate = baud,
      .data_bits = UART_DATA_5_BITS + (databits - 5),
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1 + (halfstops - 2),
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
   };
   // Configure UART parameters
   uart_param_config(uart, &uart_config);
   uart_set_pin(uart, tx ? port_mask(tx) : UART_PIN_NO_CHANGE, rx ? port_mask(rx) : UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
   uart_set_line_inverse(uart, (port_inv(rx) ? UART_SIGNAL_RXD_INV : 0) + (port_inv(tx) ? UART_SIGNAL_TXD_INV : 0));
   uart_set_rx_full_threshold(uart, 1);
}

void tty_flush(void)
{                               // Waits for all Tx to complete and returns
   if (uart < 0)
   {                            // Soft UART
      // TODO
      return;
   }
   // Hard UART
   uart_wait_tx_done(uart, portMAX_DELAY);      // Should be clear, but just in case...
}

int tty_rx_ready(void)
{                               // Reports how many bytes ready to read, 0 if none, negative for BREAK condition (after all Rx read)
   if (uart < 0)
   {                            // Soft UART
      // TODO
      return -1;
   }
   // Hard UART
   size_t len;
   uart_get_buffered_data_len(uart, &len);
   return len;
}

void tty_tx(uint8_t b)
{                               // Send a byte, blocking
   if (uart < 0)
   {                            // Soft UART
      // TODO
      return;
   }
   // Hard UART
   uart_write_bytes(uart, &b, 1);
}

uint8_t tty_rx(void)
{                               // Receive a byte, blocking
   uint8_t b = 0;
   if (uart < 0)
   {                            // Soft UART
      // TODO
      return 0;
   }
   // Hard UART
   uart_read_bytes(uart, &b, 1, 0);
   return b;
}
