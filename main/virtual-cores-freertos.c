#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_function(void *pvParameters)
{
    // Task code here
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

void app_main(void)
{
  TaskHandle_t task_0, task_1;
  xTaskCreatePinnedToCore(task_function,"Task0",2048,NULL,tskIDLE_PRIORITY,&task_0,0);
  xTaskCreatePinnedToCore(task_function,"Task1",2048,NULL,tskIDLE_PRIORITY,&task_1,1);    
  printf("Task 0 running on core %d\n", xTaskGetCoreID(task_0));
  printf("Task 1 running on core %d\n", xTaskGetCoreID(task_1));
}