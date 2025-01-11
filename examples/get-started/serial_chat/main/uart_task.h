#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#define UART_BUFFER_LENGTH 27 * 1024
enum CHAT_STATE
{
  SERIAL_INPUT,
  NET_PROCESS,
  SERIAL_OUTPUT,
};
struct UART_CHAT
{
  uint32_t length;
  char buffer[UART_BUFFER_LENGTH];
  enum CHAT_STATE state;
};
static void uart_read_line(struct UART_CHAT *chat, size_t buffer_size);
static void ParseRecvResponse(struct UART_CHAT *chat);
void uart_task(void *pvParameters);
