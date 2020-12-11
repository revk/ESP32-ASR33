// Advent stuff to work on ESP32

// Compatible functions

#include "revk.h"
#include <string.h>
#include <driver/uart.h>
#include "adventesp.h"
extern uint8_t uart;
extern void nl(void);

void sendline(const char *line, int len)
{
   if (len < 0)
      len = strlen(line);
   uart_write_bytes(uart, line, len);
}

char *readline(const char *prompt)
{
   uart_write_bytes(uart, prompt, strlen(prompt));
   char line[73];
   int p = 0;
   while (1)
   {
      usleep(10000);
      uint8_t b;
      if (uart_read_bytes(uart, &b, 1, 0) > 0)
      {
         b &= 0x7F;
         if (b == 4)
            return NULL;        // EOF
         if (b == '\r' || b == '\n')
            break;
         if (b >= ' ' && b <= 0x7F)
         {
            uart_write_bytes(uart, &b, 1);      // Echo
            line[p++] = b;
            if (p >= 72)
               break;
         }
      }
   }
   nl();
   line[p] = 0;
   return strdup(line);
}
