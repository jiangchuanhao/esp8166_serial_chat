#ifndef __OPENAI_TASK_H
#define __OPENAI_TASK_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#define OPENAI_API_KEY "sk-c2fb1a4d5c7e473da7f841ae76e92f06" // 替换为你的 OpenAI API Key
#define OPENAI_API_HOST "api.deepseek.com"
#define OPENAI_API_PATH "/chat/completions"

static esp_err_t http_event_handler(esp_http_client_event_t *evt);
void openai_api_task(void *pvParameters);

#endif