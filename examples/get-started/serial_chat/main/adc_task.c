#include "adc_task.h"
static const char *TAG = "adc_task";
void adc_task(void *pvParameters)
{
  QueueHandle_t adc_queue = pvParameters;

  uint16_t adc_data;

  while (1)
  {
    if (ESP_OK == adc_read(&adc_data))
    {
      ESP_LOGI(TAG, "adc read: %d", adc_data);

      // 将 ADC 数据发送到队列
      if (xQueueSend(adc_queue, &adc_data, portMAX_DELAY) != pdPASS)
      {
        ESP_LOGE(TAG, "Failed to send ADC data to queue");
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
