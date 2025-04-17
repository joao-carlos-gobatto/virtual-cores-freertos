#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

//xTaskIncrementTask // task.c
struct Parameters{
	int period;
	int deadline;
	TaskHandle_t handle;
};
extern struct Parameters taskParameters[5];

extern int GetTidByHandle(TaskHandle_t);
extern int tickCounter;
extern TaskHandle_t idleHandleArray[2];
void hello_task(void *pvParameter)
{
    while (1) {
    	TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();
    	int tid = GetTidByHandle(xHandle);
        printf("Hello from a FreeRTOS task!\nMy parameters are\nPeriod: %d\nDeadline: %d\n",taskParameters[tid].period, taskParameters[tid].deadline);
        
        printf("My Task ID (By FreeRtos Handle): %p\n", xHandle);
        printf("My Task ID (By GetTid): %d\n", tid);
        printf("tickCounter is %d\n", tickCounter);
        printf("Idle 0 handle: %p , Idle 1 handle: %p\n\n", idleHandleArray[0], idleHandleArray[1]);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // wait 1 second
    }
}

void app_main(void)
{
	struct Parameters pr;
	pr.period = 10;
	pr.deadline = 5;
	 vTaskDelay(1000 / portTICK_PERIOD_MS);
    xTaskCreatePinnedToCore(
        hello_task,      // Task function
        "HelloTask",     // Task name
        2048,            // Stack size (in words, not bytes)
        &pr,            // Task input parameter
        1,               // Priority
        NULL,             // Task handle
    	0			//core
    );
}