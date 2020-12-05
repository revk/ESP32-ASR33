// ASR33 direct control over MQTT
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
static const char TAG[] = "ASR33";

#include "revk.h"
#include <esp_spi_flash.h>
#include <driver/uart.h>

static char *otaurl;

const char *app_command(const char *tag, unsigned int len, const unsigned char *value)
{
   if (!strcmp(tag, "wifi") && *otaurl)
   {
      revk_ota(otaurl);
      revk_setting("otaurl", 0, NULL);  // Clear the URL
   }
   if (!strcmp(tag, "connect"))
      revk_info(TAG, "Running ASR33 control");
   if (!strcmp(tag, "tx"))
	   uart_write_bytes(0,value,len);
   return "";
}

void app_main()
{
   revk_init(&app_command);
   revk_register("otaurl", 0, 0, &otaurl, NULL, 0);
   uart_config_t uart_config = {
      .baud_rate = 110,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_2,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
   };
   uart_param_config(0, &uart_config);
   uart_set_line_inverse(0, UART_SIGNAL_RXD_INV);
   esp_log_level_set("*",ESP_LOG_ERROR);
   while (1)
   {
	   sleep(1);
   }
}
