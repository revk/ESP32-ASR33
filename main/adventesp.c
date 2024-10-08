// Advent stuff to work on ESP32

// Compatible functions

#include "revk.h"
#include <string.h>
#include "softuart.h"
#include "tty.h"
#include "adventesp.h"
extern int8_t uart;

void
pesend (const char *line, int len)
{
   while (len-- > 0)
      tty_tx (*line++);
}

void
sendline (const char *line, int len)
{
   if (len < 0)
      len = strlen (line);
   pesend (line, len);
}

char *
readline (const char *prompt)
{
   pesend (prompt, strlen (prompt));
   char line[73];
   int p = 0;
   while (1)
   {
      usleep (10000);
      char b = tty_rx ();
      b &= 0x7F;
      if (b == 4)
         return NULL;           // EOF
      if (b == 0x7F)
      {
         pesend ("\r", 2);      // \r and NULL
         pesend (prompt, strlen (prompt));
         p = 0;
         continue;
      }
      if (b == '\r' || b == '\n')
         break;
      if (b >= ' ' && b < 0x7F)
      {
         pesend (&b, 1);        // Echo
         line[p++] = b;
         if (p >= 72)
            break;
      }
   }
   line[p] = 0;
   extern uint8_t think;
   pesend ("\r\n\n", 3);
   for (int i = 0; i < think; i++)
      pesend ("", 1);
   return strdup (line);
}

void
cheese (void)
{
   sendline ("\007\r\n\200\200\200\200\200\200\200\200\200\200+++\200\200\200\200\200\200\200\200\200\200", -1);
   const char *e[] = {          // Some error messages
      "??++ OUT OF CHEESE ERROR. REDO FROM START.",     //
      "Mr. Jelly! Mr. Jelly!",  //
      "Error At Address: 14, Treacle Mine Road, Ankh-Morpork",  //
      "MELON MELON MELON",      //
      "Divide By Cucumber Error. Please Reinstall Universe And Reboot", //
      "Whoops! Here Comes The Cheese!", //
      "Oneoneoneoneoneoneone",  //
      "error code 746, divergent temporal instability", // Actually from the disorganiser not HEX but seemed appropriate
   };
   sendline (e[esp_random () % (sizeof (e) / sizeof (*e))], -1);
   sendline ("+++\r\n\r\n\007\007", -1);
}
