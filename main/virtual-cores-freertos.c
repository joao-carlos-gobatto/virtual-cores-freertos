#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"


extern struct Parameters descriptors[120];

struct Parameters tasks[20];
extern int GetTidByHandle(TaskHandle_t);
extern int tickCounter;
extern TaskHandle_t idleHandleArray[2];

extern int task_id_buffer[60];
extern int task_id_buffer_0[60];
extern int task_id_buffer_1[60];
extern TaskHandle_t task_handle_buffer[60];
extern int task_id_buffer_index;
int task_id_buffer_start = 0;
extern int task_id_buffer_index_0;
int task_id_0_buffer_start = 0;
extern int task_id_buffer_index_1;
int task_id_1_buffer_start = 0;

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

#define BUFFER_SIZE 2048
char string_buffer[BUFFER_SIZE];
int buffer_index = 0;

void addToStringBuffer(const char* str) {
    int len = strlen(str);
    if (buffer_index + len < BUFFER_SIZE - 1) {
        strcpy(&string_buffer[buffer_index], str);
        buffer_index += len;
        string_buffer[buffer_index++] = '\n';  // Add newline after each entry
        string_buffer[buffer_index] = '\0';    // Null-terminate
    }
    else {
        string_buffer[BUFFER_SIZE - 2] = 'Z';
        string_buffer[BUFFER_SIZE - 1] = '\0';
        buffer_index = BUFFER_SIZE - 1;    
    }
}

void printAndClearStringBuffer() {
    if (buffer_index > 0) {
        printf("Buffered Output:\n%s", string_buffer);
        buffer_index = 0;
        string_buffer[0] = '\0';
    }
    else {
        printf("Buffer is empty.\n");
    }
}



void initVirtualCores(){
    if(getSchedulingAlgorithm() == RMC){
        int virtualCoreZero = 0,virtualCoreOne = 0,virtualCoreTwo = 0,virtualCoreThree = 0;
        //Pensar em uma forma de contar a quantidade de task não nulas para entrar no for.
        for (size_t i = 0; i < 10; i++)
        {
            tasks[i].task_virtual_core = i%4;
        }
        for (size_t i = 0; i < 10; i++)
        {
            xTaskCreatePinnedToCore(
                tasks[i].task_function,
                tasks[i].task_name,
                2048,
                &tasks[i],
                0,
                NULL,
                i%2
            );
        }
    } else if(getSchedulingAlgorithm() == RRC) {
        //Usuário pode ou não selecionar o core da task.
    } else {
        //Algoritmo selecionado é o EDF, então será uma lista global.
    }
}

void printTaskIdsBuffer(void) {
    int count = (task_id_buffer_index < 60) ? task_id_buffer_index : 60;
    int start = (task_id_buffer_index < 60) ? 0 : task_id_buffer_index % 60;
    int start_0 = (task_id_buffer_index_0 < 60) ? 0 : task_id_0_buffer_start % 60;
    int start_1 = (task_id_buffer_index_1 < 60) ? 0 : task_id_1_buffer_start % 60;

    for (int i = 0; i < count; i++) {
        int index = (start + i) % 60;
        int index_0 = (start_0 + i) % 60;
        int index_1 = (start_1 + i) % 60;
        // printf("Task ID: %d, handle: %p\n", task_id_buffer[index], task_handle_buffer[index]);
        printf("Task ID Selected at Core 0: %d\tTask ID Selected at Core 1: %d\n", task_id_buffer_0[index_0], task_id_buffer_1[index_1]);
    }
}

void print_task(){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while(1){
        printf("Virtual Core ReadyList Shifting\nCore 0: %d, Core 1: %d\n", getCount_do_celsinho_manobrown_0(),getCount_do_celsinho_manobrown_1());
        printf("-------------------------------------------------------------------------------------------------\n");
        printf("| Task ID | Period  | Computing Time | Period Dyn | Computing Time Dyn | Core ID | Tick Counter |\n");
        printf("-------------------------------------------------------------------------------------------------\n");
        for (size_t i = 0; i < 8; i++) {
            printf("| %-7d | %-7d | %-14d | %-10d | %-17d | %-7d | %-12d |\n",
                i,
                descriptors[i].period,
                descriptors[i].computing_time,
                descriptors[i].period_dynamic,
                descriptors[i].computing_time_dynamic,
                descriptors[i].task_core,
                tickCounter);
        }
        printf("---------------------------------------------------------------------------------------------------\n");
        printf("Idle 0 handle: %p , Idle 1 handle: %p\n", idleHandleArray[0], idleHandleArray[1]);
        printf("History of selected tasks:\n");
        printTaskIdsBuffer();
        printAndClearStringBuffer();
        vTaskDelay(600 / portTICK_PERIOD_MS);
    }
}

void hello_task(void *pvParameter)
{
    struct Parameters* params = (struct Parameters*)pvParameter;
    while (1) {
    	// TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();
        // printf("HELLO TASK\n");
        //printf("Task ID: %d\n", params->period);
        char line[16]; 
         
        addToStringBuffer("Hello Task Computing Time: ");
        snprintf(line, sizeof(line), "%d", params->computing_time);
        addToStringBuffer(line);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // wait 1 second
    }
}

void app_main(void)
{
    // struct Parameters pr,pr1,pr2,pr3,prprint;

    setSchedulingAlgorithm(RMC);

    // tasks[0].period = 1000;
    // tasks[0].task_function = print_task;
    // tasks[0].task_name = strdup("Print Task");  // Don't forget to free later
    // tasks[0].computing_time = 5;
    // tasks[0].final_task = 0;

    tasks[0].period = 1000;
    tasks[0].task_function = hello_task;
    tasks[0].task_name = strdup("Hello Task 0");  // Requires char* not char[]
    tasks[0].computing_time = 5;
    tasks[0].final_task = 0;

    tasks[1].period = 1000;
    tasks[1].task_function = hello_task;
    tasks[1].task_name = strdup("Hello Task 1");  // Requires char* not char[]
    tasks[1].computing_time = 10;
    tasks[1].final_task = 0;

    tasks[2].period = 1000;
    tasks[2].task_function = hello_task;
    tasks[2].task_name = strdup("Hello Task 2");  // Requires char* not char[]
    tasks[2].computing_time = 15;
    tasks[2].final_task = 0;

    tasks[3].period = 1000;
    tasks[3].task_function = hello_task;
    tasks[3].task_name = strdup("Hello Task 3");  // Requires char* not char[]
    tasks[3].computing_time = 20;
    tasks[3].final_task = 0;

    tasks[4].period = 1000;
    tasks[4].task_function = hello_task;
    tasks[4].task_name = strdup("Hello Task 4");  // Requires char* not char[]
    tasks[4].computing_time = 25;
    tasks[4].final_task = 0;

    tasks[5].period = 1000;
    tasks[5].task_function = hello_task;
    tasks[5].task_name = strdup("Hello Task 5");  // Requires char* not char[]
    tasks[5].computing_time = 30;
    tasks[5].final_task = 0;

    tasks[6].period = 1000;
    tasks[6].task_function = hello_task;
    tasks[6].task_name = strdup("Hello Task 6");  // Requires char* not char[]
    tasks[6].computing_time = 35;
    tasks[6].final_task = 0;

    tasks[7].period = 1000;
    tasks[7].task_function = hello_task;
    tasks[7].task_name = strdup("Hello Task 7");  // Requires char* not char[]
    tasks[7].computing_time = 40;
    tasks[7].final_task = 0;

    tasks[8].period = 1000;
    tasks[8].task_function = hello_task;
    tasks[8].task_name = strdup("Hello Task 8");  // Requires char* not char[]
    tasks[8].computing_time = 45;
    tasks[8].final_task = 0;

    tasks[9].period = 1000;
    tasks[9].task_function = hello_task;
    tasks[9].task_name = strdup("Hello Task 9");  // Requires char* not char[]
    tasks[9].computing_time = 50;
    tasks[9].final_task = 1;

    xTaskCreate(
        print_task,           // Function that implements the task
        "PrintTask",          // Text name for debugging
        2048, // Stack size in words
        NULL,                 // Task input parameter
        tskIDLE_PRIORITY + 1, // Priority of the task
        NULL                  // Task handle
    );
    initVirtualCores();
}