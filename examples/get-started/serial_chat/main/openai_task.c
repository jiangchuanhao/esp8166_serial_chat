#include "openai_task.h"
#include "uart_task.h"
#include "cJSON.h"
static const char *TAG = "OPENAI_API_EXAMPLE";

#define MAX_JSON_LENGTH 10 * 1024 // JSON 数据的最大长度

// 动态生成 OpenAI API 请求的 JSON 数据

void generate_openai_request(char *json_buffer, size_t buffer_size, const char *user_content)
{
  cJSON *root = cJSON_CreateObject();
  cJSON *messages = cJSON_CreateArray();
  cJSON *message = cJSON_CreateObject();

  // 构建 JSON 数据
  cJSON_AddStringToObject(root, "model", "deepseek-chat");
  cJSON_AddItemToArray(messages, message);
  cJSON_AddStringToObject(message, "role", "user");
  cJSON_AddStringToObject(message, "content", user_content); // 自动处理转义
  cJSON_AddItemToObject(root, "messages", messages);
  cJSON_AddBoolToObject(root, "stream", false);

  // 将 JSON 数据转换为字符串
  char *json_str = cJSON_PrintUnformatted(root);
  snprintf(json_buffer, buffer_size, "%s", json_str);

  // 释放 cJSON 对象
  cJSON_Delete(root);
  free(json_str);
}
#include "cJSON.h"

static uint16_t parse_response(const char *json_str, char *buffer)
{
  uint16_t length;
  cJSON *root = cJSON_Parse(json_str);
  if (root == NULL)
  {
    ESP_LOGE(TAG, "Failed to parse JSON");
    return 0;
  }

  // 解析 choices 字段
  cJSON *choices = cJSON_GetObjectItem(root, "choices");
  if (choices != NULL && cJSON_IsArray(choices))
  {
    cJSON *choice = cJSON_GetArrayItem(choices, 0);
    if (choice != NULL)
    {
      // 解析 delta.content
      cJSON *delta = cJSON_GetObjectItem(choice, "delta");
      if (delta != NULL)
      {
        cJSON *content = cJSON_GetObjectItem(delta, "content");
        if (content != NULL && cJSON_IsString(content))
        {
          ESP_LOGI(TAG, "Content: %s", content->valuestring);
          memcpy(buffer, content->valuestring, strlen(content->valuestring));
          length = strlen(content->valuestring);
          cJSON_Delete(root);
          return length;
        }
        else
        {
        }
      }

      // 解析 finish_reason
      cJSON *finish_reason = cJSON_GetObjectItem(choice, "finish_reason");
      if (finish_reason != NULL && cJSON_IsString(finish_reason))
      {
        ESP_LOGI(TAG, "Finish Reason: %s", finish_reason->valuestring);
      }
    }
  }

  // 解析 usage 字段
  // cJSON *usage = cJSON_GetObjectItem(root, "usage");
  // if (usage != NULL)
  // {
  //   cJSON *prompt_tokens = cJSON_GetObjectItem(usage, "prompt_tokens");
  //   cJSON *completion_tokens = cJSON_GetObjectItem(usage, "completion_tokens");
  //   cJSON *total_tokens = cJSON_GetObjectItem(usage, "total_tokens");

  //   if (prompt_tokens != NULL && cJSON_IsNumber(prompt_tokens))
  //   {
  //     ESP_LOGI(TAG, "Prompt Tokens: %d", prompt_tokens->valueint);
  //   }
  //   if (completion_tokens != NULL && cJSON_IsNumber(completion_tokens))
  //   {
  //     ESP_LOGI(TAG, "Completion Tokens: %d", completion_tokens->valueint);
  //   }
  //   if (total_tokens != NULL && cJSON_IsNumber(total_tokens))
  //   {
  //     ESP_LOGI(TAG, "Total Tokens: %d", total_tokens->valueint);
  //   }
  // }

  // 释放 cJSON 对象
  cJSON_Delete(root);
  return 0;
}
// HTTP 事件处理函数
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
  int mbedtls_err = 0;
  struct UART_CHAT *chat = evt->user_data;
  static int output_len = 0; // Stores number of bytes read
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    /*
     *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
     *  However, event handler can also be used in case chunked encoding is used.
     */
    // If user_data buffer is configured, copy the response into the buffer
    if (output_len + evt->data_len > UART_BUFFER_LENGTH)
    {

      *(char *)(chat->buffer + output_len + 1) = '\0';
      // ESP_LOGI(TAG, "%s", chat->buffer);
      chat->length = output_len;
      ESP_LOGE(TAG, "Received specific data, closing connection...");
      esp_http_client_close(evt->client); // 主动关闭连接

      chat->state = SERIAL_OUTPUT;
      output_len = 0;
    }
    memcpy(chat->buffer + output_len, evt->data, evt->data_len);
    output_len += evt->data_len;
    // output_len = parse_response(evt->data, evt->user_data + output_len);
    // ESP_LOGI(TAG, "recev buffer %.*s", evt->data_len, (char *)evt->data);

    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");

    if (evt->user_data)
    {
      *(char *)(chat->buffer + output_len) = '\0';
      chat->length = output_len;
      ets_printf("%s", chat->buffer);
      // os_printf("%.*s", chat->length, chat->buffer);

      chat->state = SERIAL_OUTPUT;
      output_len = 0;
      esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
    }
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");

    esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
    if (err != 0)
    {

      ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
      ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
    }
    break;
  }
  return ESP_OK;
}
void openai_api_task(void *pvParameters)
{
  char *json_buffer;
  size_t json_buffer_size;

  struct UART_CHAT *chat = (struct UART_CHAT *)pvParameters;
  esp_http_client_handle_t client;
  // 配置 HTTP 客户端
  esp_http_client_config_t config = {
      .host = OPENAI_API_HOST,
      .path = OPENAI_API_PATH,
      .method = HTTP_METHOD_POST,
      .event_handler = http_event_handler,
      .transport_type = HTTP_TRANSPORT_OVER_SSL, // 使用 HTTPS
      .skip_cert_common_name_check = true,
      .user_data = chat,
      .timeout_ms = 200000, // 将超时时间增加到 100 秒
                            // .is_async = true,
  };

  while (1)
  {
    // 假设 chat 是一个指向结构体的指针
    if (chat->state == NET_PROCESS)
    {
      // 分配 JSON 缓冲区
      json_buffer_size = chat->length + 200; // 确保缓冲区足够大
      json_buffer = malloc(json_buffer_size);
      if (json_buffer == NULL)
      {
        ESP_LOGE(TAG, "json buffer malloc failed: %d", json_buffer_size);
        continue; // 或者进行其他错误处理
      }

      // 生成 JSON 数据
      generate_openai_request(json_buffer, json_buffer_size, chat->buffer);
      ESP_LOGI(TAG, "json buffer: %s", json_buffer);
      chat->length = 0;
      // 初始化 HTTP 客户端
      client = esp_http_client_init(&config);
      if (client == NULL)
      {
        ESP_LOGE(TAG, "client init failed");
        vTaskDelete(NULL);
      }
      // 设置请求头
      esp_http_client_set_header(client, "Content-Type", "application/json");
      esp_http_client_set_header(client, "Authorization", "Bearer " OPENAI_API_KEY);
      esp_http_client_set_header(client, "Connection", "keep-alive");
      // 设置请求体
      esp_http_client_set_post_field(client, json_buffer, strlen(json_buffer));

      // printf("Free Heap Size: %d bytes\n", esp_get_free_heap_size());
      // 执行 HTTP 请求
      esp_err_t err = esp_http_client_perform(client);
      // printf("Free Heap Size: %d bytes\n", esp_get_free_heap_size());
      ESP_LOGI(TAG, "esp_http_client_perform");
      if (err == ESP_OK)
      {
        ESP_LOGI(TAG, "HTTP request status: %d", esp_http_client_get_status_code(client));
      }
      else
      {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
      }
      // 释放 JSON 缓冲区
      free(json_buffer);
      // 清理 HTTP 客户端
      esp_http_client_cleanup(client);
      // printf("Free Heap Size: %d bytes\n", esp_get_free_heap_size());
      // 等待 10 秒后重试
      // ESP_LOGI(TAG, "Waiting 10 seconds before next request...");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}
