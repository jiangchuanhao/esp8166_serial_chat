#include "uart_task.h"
static const char *TAG = "uart_task";

static void uart_read_line(struct UART_CHAT *chat, size_t buffer_size)
{
  uint16_t pos = 0;
  uint16_t len;
  TickType_t last_received_time;
  while (1)
  {
    uint8_t data[10];
    if (chat->state == SERIAL_INPUT)
    {
      if (pos == 0)
      {

        len = uart_read_bytes(UART_NUM_0, data, 1, portMAX_DELAY);
        last_received_time = xTaskGetTickCount();
      }
      else
      {
        len = uart_read_bytes(UART_NUM_0, data, 1, pdMS_TO_TICKS(50)); // 10ms 超时
      }
      if (len > 0)
      {
        last_received_time = xTaskGetTickCount();

        if (pos < buffer_size - 2)
        { // 防止缓冲区溢出
          if ((data[0] == '\n') || (data[0] == '\r'))
          {
            chat->buffer[pos++] = '\n';
          }
          else
          {
            chat->buffer[pos++] = data[0];
          }
        }
        else
        {
          chat->buffer[pos] = '\0'; // 添加字符串结束符
          break;
        }
      }
      else if (xTaskGetTickCount() - last_received_time > pdMS_TO_TICKS(100)) // 100ms超时
      {
        chat->buffer[pos] = '\0'; // 添加字符串结束符
        // printf("file=%s\tline=%d\r\n", __FILE__, __LINE__);
        break;
      }
    }
  }
  chat->length = pos;
}
// 任务：从串口读取数据并打印
int init = 0;
void uart_task(void *pvParameters)
{
  struct UART_CHAT *chat = (struct UART_CHAT *)pvParameters;

  while (1)
  {
    // 读取一行数据
    if (chat->state == SERIAL_INPUT)
    {
      if (init == 0)
      {
        printf("\r\n初始化完成\r\n");
        init = 1;
      }

      uart_read_line(chat, UART_BUFFER_LENGTH);
      // 打印输入的数据
      chat->state = NET_PROCESS;
      // printf("user:%s\r\n", chat->buffer);
    }
    else if (chat->state == SERIAL_OUTPUT)
    {
      // ParseRecvResponse(chat);
      // printf("%s", chat->buffer);
      chat->state = SERIAL_INPUT;
    }
  }
}
static void ParseRecvResponse(struct UART_CHAT *chat)
{
  uint32_t i, start, end;
  i = 0;
  start = 0;
  end = 0;

  // 查找 "content": 字段的起始位置
  while (i < chat->length - 10)
  {
    if (!strncmp(chat->buffer + i, "\"content\":\"", 11))
    {
      start = i + 11;
      break;
    }
    i++;
  }

  // 如果没有找到 "content": 字段，直接返回
  if (start == 0)
  {
    printf("Error: 'content' field not found.\n");
    return;
  }

  // 查找 "},\"logprobs\"" 字段的结束位置
  i = start;
  while (i < chat->length - 15)
  {
    if (!strncmp(chat->buffer + i, "\"},\"logprobs\"", 13))
    {
      end = i;
      break;
    }
    i++;
  }

  // 如果没有找到 "},\"logprobs\"" 字段，直接返回
  if (end == 0)
  {
    printf("Error: 'logprobs' field not found.\n");
    return;
  }

  // 输出 content 字段的值
  // printf("Start: %d, End: %d\n", start, end);
  printf("%.*s\n", end - start, chat->buffer + start);
}