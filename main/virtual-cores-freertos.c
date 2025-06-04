#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"


extern struct Parameters descriptors[MAX_NUMBER_TASK_C];

struct Parameters tasks[20];
extern int GetTidByHandle(TaskHandle_t);
extern int tickCounter;
extern TaskHandle_t idleHandleArray[2];

int task_count_celsinho_mano = 0;

extern int task_id_buffer[BUFFER_SIZE_C];
extern int task_id_buffer_0[BUFFER_SIZE_C];
extern int task_id_buffer_1[BUFFER_SIZE_C];
extern TaskHandle_t task_handle_buffer[BUFFER_SIZE_C];
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
    if(getSchedulingAlgorithm() == RMC || getSchedulingAlgorithm() == RRC){
        //Pensar em uma forma de contar a quantidade de task não nulas para entrar no for.
        for (size_t i = 0; i < 8; i++)
        {
            //tasks[i].task_virtual_core = i%VIRTUAL_CORE_QUANTITY_C;
        }
        for (size_t i = 0; i < 8; i++)
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
    int count = (task_id_buffer_index < BUFFER_SIZE_C) ? task_id_buffer_index : BUFFER_SIZE_C;
    int start = (task_id_buffer_index < BUFFER_SIZE_C) ? 0 : task_id_buffer_index % BUFFER_SIZE_C;
    int start_0 = (task_id_buffer_index_0 < BUFFER_SIZE_C) ? 0 : task_id_0_buffer_start % BUFFER_SIZE_C;
    int start_1 = (task_id_buffer_index_1 < BUFFER_SIZE_C) ? 0 : task_id_1_buffer_start % BUFFER_SIZE_C;

    for (int i = 0; i < count; i++) {
        int index = (start + i) % BUFFER_SIZE_C;
        int index_0 = (start_0 + i) % BUFFER_SIZE_C;
        int index_1 = (start_1 + i) % BUFFER_SIZE_C;
        // printf("Task ID: %d, handle: %p\n", task_id_buffer[index], task_handle_buffer[index]);
        printf("Virtual core at Core 0: %d\tVirtual core at Core 1: %d\n", task_id_buffer_0[index_0], task_id_buffer_1[index_1]);
    }
}

void print_task(){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while(1){
        printf("Virtual Core ReadyList Shifting\nCore 0: %ld, Core 1: %ld\n", getCount_do_celsinho_manobrown_0(),getCount_do_celsinho_manobrown_1());
        printf("-------------------------------------------------------------------------------------------------\n");
        printf("| Task ID | Period  | Computing Time | Period Dyn | Computing Time Dyn | Core ID | Tick Counter |\n");
        printf("-------------------------------------------------------------------------------------------------\n");
        for (size_t i = 0; i < task_count_celsinho_mano; i++) {
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
        //vTaskDelay(1000 / portTICK_PERIOD_MS); // Se usar vTaskDelay, o tempo de comp dinamico não é decrementado nas tasks no core real 1
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

    xTaskCreatePinnedToCore(
        print_task,           // Function that implements the task
        "PrintTask",          // Text name for debugging
        2048,                 // Stack size in words
        NULL,                 // Task input parameter
        tskIDLE_PRIORITY + 1, // Priority of the task
        NULL,                  // Task handle
        0
    );

    tasks[0].period = 1000;
    tasks[0].task_function = hello_task;
    tasks[0].task_name = strdup("Hello Task 0");  // Requires char* not char[]
    tasks[0].computing_time = 5;
    tasks[0].final_task = 0;
    tasks[0].task_virtual_core = 0;

    tasks[1].period = 999;
    tasks[1].task_function = hello_task;
    tasks[1].task_name = strdup("Hello Task 1");  // Requires char* not char[]
    tasks[1].computing_time = 10;
    tasks[1].final_task = 0;
    tasks[1].task_virtual_core = 0;

    tasks[2].period = 999;
    tasks[2].task_function = hello_task;
    tasks[2].task_name = strdup("Hello Task 2");  // Requires char* not char[]
    tasks[2].computing_time = 15;
    tasks[2].final_task = 0;
    tasks[2].task_virtual_core = 2;

    tasks[3].period = 1000;
    tasks[3].task_function = hello_task;
    tasks[3].task_name = strdup("Hello Task 3");  // Requires char* not char[]
    tasks[3].computing_time = 20;
    tasks[3].final_task = 0;
    tasks[3].task_virtual_core = 2;

    tasks[4].period = 1000;
    tasks[4].task_function = hello_task;
    tasks[4].task_name = strdup("Hello Task 4");  // Requires char* not char[]
    tasks[4].computing_time = 25;
    tasks[4].final_task = 0;
    tasks[4].task_virtual_core = 1;

    tasks[5].period = 999;
    tasks[5].task_function = hello_task;
    tasks[5].task_name = strdup("Hello Task 5");  // Requires char* not char[]
    tasks[5].computing_time = 30;
    tasks[5].final_task = 0;
    tasks[5].task_virtual_core = 1;

    tasks[6].period = 999;
    tasks[6].task_function = hello_task;
    tasks[6].task_name = strdup("Hello Task 6");  // Requires char* not char[]
    tasks[6].computing_time = 35;
    tasks[6].final_task = 0;
    tasks[6].task_virtual_core = 3;

    tasks[7].period = 1000;
    tasks[7].task_function = hello_task;
    tasks[7].task_name = strdup("Hello Task 6");  // Requires char* not char[]
    tasks[7].computing_time = 40;
    tasks[7].final_task = 1;
    tasks[7].task_virtual_core = 3;



    // for (size_t i = 0; i < (MAX_NUMBER_TASK_C/4); i++)
    // {
    //     tasks[0].period = 1000;
    //     tasks[0].task_function = hello_task;
    //     tasks[0].task_name = strdup("Hello Task");  // Requires char* not char[]
    //     tasks[0].computing_time = 2*(i+1);
    //     if(i == (MAX_NUMBER_TASK_C/4) - 1){
    //         tasks[i].final_task = 1;
    //     } else {
    //         tasks[i].final_task = 0;
    //     }
    // }

    initVirtualCores();
}