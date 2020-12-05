// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
static const char TAG[] = "ASR33";

#include "revk.h"
#include <esp_spi_flash.h>
#include <driver/uart.h>


#define settings  \
  u8(uart,1);	\
  u8(tx,1);	\
  u8(rx,3);	\
  t(sonoff);	\
  u32(idle,60)	\

#define u32(n,d) uint32_t n;
#define u16(n,d) uint16_t n;
#define u8(n,d) uint8_t n;
#define u1(n) uint8_t n;
#define t(n) const char*n=NULL;
settings
#undef t
#undef u32
#undef u16
#undef u8
#undef u1
    uint32_t last;

void turn_off(void)
{
   last = 0;
   if (sonoff)
      revk_raw(NULL, sonoff, 1, "0", 0);
}

void turn_on(void)
{
   if (last)
      return;
   last = time(0);
   if (sonoff)
      revk_raw(NULL, sonoff, 1, "1", 0);
}

const char *app_command(const char *tag, unsigned int len, const unsigned char *value)
{
   if (!strcmp(tag, "connect"))
   {
      revk_info(TAG, "Running ASR33 control");
      turn_off();
   }
   if (!strcmp(tag, "upgrade"))
      turn_off();
   if (!strcmp(tag, "on"))
      turn_on();
   if (!strcmp(tag, "off"))
      turn_off();
   if (!strcmp(tag, "tx"))
   {
      turn_on();
      uart_write_bytes(uart, value, len);
      last = time(0);
   }
   return "";
}

void app_main()
{
   revk_init(&app_command);
#define u32(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u16(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u8(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define u1(n) revk_register(#n,0,sizeof(n),&n,NULL,SETTING_BOOLEAN);
#define t(n) revk_register(#n,0,0,&n,NULL,0);
   settings
#undef t
#undef u32
#undef u16
#undef u8
#undef u1
       turn_off();
   ESP_ERROR_CHECK(uart_driver_install(uart, 1024, 1024, 0, NULL, 0));
   uart_config_t uart_config = {
      .baud_rate = 110,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_2,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
   };
// Configure UART parameters
   ESP_ERROR_CHECK(uart_param_config(uart, &uart_config));
   ESP_ERROR_CHECK(uart_set_pin(uart, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

   while (1)
   {
      usleep(90000);
      time_t now = time(0);
      if (last && now > last + idle)
         turn_off();
      // TODO ready byte
   }
}
