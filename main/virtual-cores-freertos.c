#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

//xTaskIncrementTask // task.c
enum scheduling_algorithms{
    RR,
    EDF,
    RM
};
struct Parameters{
	int period;
    int period_dynamic;
	int computing_time;
    int computing_time_dynamic;
	TaskHandle_t handle;
    int task_number;
    int final_task;
    int scheduling_algorithm;
};

extern struct Parameters descriptors[5];

extern int GetTidByHandle(TaskHandle_t);
extern int tickCounter;
extern TaskHandle_t idleHandleArray[2];
void hello_task(void *pvParameter)
{
    while (1) {
    	TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();
    	int tid = GetTidByHandle(xHandle);
        printf("Hello from a FreeRTOS task!\nMy parameters are\nPeriod: %d\nDeadline: %d\n",descriptors[tid].period, descriptors[tid].computing_time);
        
        printf("My Task ID (By FreeRtos Handle): %p\n", xHandle);
        printf("My Task ID (By GetTid): %d\n", tid);
        printf("tickCounter is %d\n", tickCounter);
        printf("Idle 0 handle: %p , Idle 1 handle: %p\n\n", idleHandleArray[0], idleHandleArray[1]);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // wait 1 second
    }
}

void app_main(void)
{
	struct Parameters pr,pr1,pr2,pr3;
    pr.period = 10;
    pr.computing_time = 5;
    pr.final_task = 0;
    pr.scheduling_algorithm = RM;

    pr1.period = 20;
    pr1.computing_time = 10;
    pr1.final_task = 0;
    pr1.scheduling_algorithm = RM;

    pr2.period = 30;
    pr2.computing_time = 15;
    pr2.final_task = 0;
    pr2.scheduling_algorithm = RM;

    pr3.period = 40;
    pr3.computing_time = 20;
    pr3.final_task = 1;
    pr3.scheduling_algorithm = RM;

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
    xTaskCreatePinnedToCore(
        hello_task,      // Task function
        "HelloTask1",     // Task name
        2048,            // Stack size (in words, not bytes)
        &pr1,            // Task input parameter
        1,               // Priority
        NULL,             // Task handle
    	0			//core
    );
    xTaskCreatePinnedToCore(
        hello_task,      // Task function
        "HelloTask2",     // Task name
        2048,            // Stack size (in words, not bytes)
        &pr2,            // Task input parameter
        1,               // Priority
        NULL,             // Task handle
    	0			//core
    );
    xTaskCreatePinnedToCore(
        hello_task,      // Task function
        "HelloTask3",     // Task name
        2048,            // Stack size (in words, not bytes)
        &pr3,            // Task input parameter
        1,               // Priority
        NULL,             // Task handle
    	0			//core
    );
}