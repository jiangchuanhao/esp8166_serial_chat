/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_err.h"

#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <netdb.h>
#include <sys/socket.h>

#include "uart_task.h"
#include "adc_task.h"
#include "openai_task.h"
static char *TAG = "main_task";
void app_main()
{

  struct UART_CHAT *chat;
  chat = (struct UART_CHAT *)malloc(sizeof(struct UART_CHAT));
  if (chat == NULL)
  {
    ESP_LOGE(TAG, "chat malloc failed\r\n");
    while (1)
      ;
  }
  chat->length = 0;
  chat->state = SERIAL_INPUT;

  // connect to wifi
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(example_connect());
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
  uart_param_config(UART_NUM_0, &uart_config);
  uart_driver_install(UART_NUM_0, 150, 0, 0, NULL, 0);
  // 2. Create a adc task to read adc value
  // xTaskCreate(adc_task, "adc_task", 1024, adc_queue, 5, NULL);

  xTaskCreate(uart_task, "uart_input_task", 1024, chat, 5, NULL);
  xTaskCreate(openai_api_task, "openai_api_task", 4 * 1024, chat, 5, NULL);
}
