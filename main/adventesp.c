// Advent stuff to work on ESP32

// Compatible functions

#include "revk.h"
#include <string.h>
#include <driver/uart.h>
#include "adventesp.h"
extern uint8_t uart;
extern void nl(void);

static inline uint8_t pe(uint8_t b)
{                               // Make even parity
   b &= 0x7F;
   for (int i = 0; i < 7; i++)
      if (b & (1 << i))
         b ^= 0x80;
   return b;
}

void pesend(const char *line, int len)
{
   while (len-- > 0)
   {
      uint8_t b = pe(*line++);
      uart_write_bytes(uart, &b, 1);
   }
}

void sendline(const char *line, int len)
{
   if (len < 0)
      len = strlen(line);
   pesend(line, len);
}

char *readline(const char *prompt)
{
   pesend(prompt, strlen(prompt));
   char line[73];
   int p = 0;
   while (1)
   {
      usleep(10000);
      char b;
      if (uart_read_bytes(uart, &b, 1, 0) > 0)
      {
         b &= 0x7F;
         if (b == 4)
            return NULL;        // EOF
         if (b == 0x7F)
         {
            pesend("\r", 2);    // \r and NULL
            pesend(prompt, strlen(prompt));
            p = 0;
            continue;
         }
         if (b == '\r' || b == '\n')
            break;
         if (b >= ' ' && b <= 0x7F)
         {
            pesend(&b, 1);      // Echo
            line[p++] = b;
            if (p >= 72)
               break;
         }
      }
   }
   nl();
   line[p] = 0;
   extern uint8_t think;
   pesend("\r\n", 2);
   for (int i = 0; i < think; i++)
      pesend("", 1);
   return strdup(line);
}
