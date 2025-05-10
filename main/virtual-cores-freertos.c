#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"


extern struct Parameters descriptors[5];

extern int GetTidByHandle(TaskHandle_t);
extern int tickCounter;
extern TaskHandle_t idleHandleArray[2];

const char* getTaskStateName(int state) {
    switch (state) {
        case eRunning:   return "Running";
        case eReady:     return "Ready";
        case eBlocked:   return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted:   return "Deleted";
        case eInvalid:   return "Invalid";
        default:         return "Unknown State";
    }
}

void print_task(){
    while(1){
        printf("---------------------------------------------------------------------------------------------------------------\n");
        printf("| Task ID | Period  | Computing Time | Period Dyn | Computing Time Dyn | Core ID | Task Status | Tick Counter |\n");
        printf("---------------------------------------------------------------------------------------------------------------\n");
        for (size_t i = 0; i < 5; i++) {
            printf("| %-7d | %-7d | %-14d | %-10d | %-17d | %-7d | %-11s | %-12d |\n",
                i,
                descriptors[i].period,
                descriptors[i].computing_time,
                descriptors[i].period_dynamic,
                descriptors[i].computing_time_dynamic,
                descriptors[i].task_core,
                getTaskStateName(eTaskGetState(descriptors[i].handle)),
                tickCounter);
        }
        printf("---------------------------------------------------------------------------------------------------------------\n");
        printf("Idle 0 handle: %p , Idle 1 handle: %p\n", idleHandleArray[0], idleHandleArray[1]);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

void hello_task(void *pvParameter)
{
    while (1) {
    	// TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();
        vTaskDelay(1000 / portTICK_PERIOD_MS); // wait 1 second
    }
}

void app_main(void)
{
    struct Parameters pr,pr1,pr2,pr3,prprint;

    setSchedulingAlgorithm(RMC);

    prprint.period = 1000;
    prprint.computing_time = 5;
    prprint.final_task = 0;

    pr.period = 1000;
    pr.computing_time = 5;
    pr.final_task = 0;

    pr1.period = 1000;
    pr1.computing_time = 10;
    pr1.final_task = 0;

    pr2.period = 1000;
    pr2.computing_time = 15;
    pr2.final_task = 0;

    pr3.period = 1000;
    pr3.computing_time = 20;
    pr3.final_task = 1;

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    xTaskCreatePinnedToCore(
        print_task,     // Task function
        "Print Task",   // Task name
        2048,           // Stack size (in words, not bytes)
        &prprint,       // Task input parameter
        0,              // Priority
        NULL,           // Task handle
    	0               // Core
    );

    xTaskCreatePinnedToCore(
        hello_task,     // Task function
        "HelloTask0",   // Task name
        2048,           // Stack size (in words, not bytes)
        &pr,            // Task input parameter
        0,              // Priority
        NULL,           // Task handle
    	0   // Core
    );
    xTaskCreatePinnedToCore(
        hello_task,     // Task function
        "HelloTask1",   // Task name
        2048,           // Stack size (in words, not bytes)
        &pr1,           // Task input parameter
        0,              // Priority
        NULL,           // Task handle
    	0               // Core
    );
    xTaskCreatePinnedToCore(
        hello_task,     // Task function
        "HelloTask2",   // Task name
        2048,           // Stack size (in words, not bytes)
        &pr2,           // Task input parameter
        0,              // Priority
        NULL,           // Task handle
    	1               // Core
    );
    xTaskCreatePinnedToCore(
        hello_task,     // Task function
        "HelloTask3",   // Task name
        2048,           // Stack size (in words, not bytes)
        &pr3,           // Task input parameter
        0,              // Priority
        NULL,           // Task handle
    	1               // Core
    );
}