#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"


extern struct Parameters descriptors[MAX_NUMBER_TASK_C];

struct Parameters tasks[MAX_NUMBER_TASK_C];
extern int GetTidByHandle(TaskHandle_t);
extern void printAllReadyLists();
extern int tickCounter;
extern TaskHandle_t idleHandleArray[2];

int total_task_count = 0;
int task_count = 0;

extern int task_id_buffer[BUFFER_SIZE_C];
extern int gantt_buffer_0[BUFFER_SIZE_C][4];
extern int gantt_buffer_1[BUFFER_SIZE_C][4];
extern TaskHandle_t task_handle_buffer[BUFFER_SIZE_C];
extern int task_id_buffer_index;
int task_id_buffer_start = 0;
extern int gantt_buffer_index_0;
int task_id_0_buffer_start = 0;
extern int gantt_buffer_index_1;
int task_id_1_buffer_start = 0;

extern uint32_t start_scheduler_timer, end_scheduler_timer;

SemaphoreHandle_t xSemaphore;

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

static char string_buffer[2][BUFFER_SIZE];
static int  buffer_index[2] = {0, 0};

void addToStringBuffer(const char* str) {
    int core = esp_cpu_get_core_id();      
    if (core < 0 || core >= 2) {
        // 
        return;
    }

    int len = strlen(str);
    int idx = buffer_index[core];

    if (idx + len < BUFFER_SIZE - 1) {
        strcpy(&string_buffer[core][idx], str);
        idx += len;
        string_buffer[core][idx++] = '\n';
        string_buffer[core][idx]   = '\0';
        buffer_index[core] = idx;
    } else {
        string_buffer[core][BUFFER_SIZE - 2] = 'Z';
        string_buffer[core][BUFFER_SIZE - 1] = '\0';
        buffer_index[core] = BUFFER_SIZE - 1;
    }
}

void printAndClearStringBuffer(int core) {
    if (core < 0 || core >= 2) {
        printf("Invalid core %d\n", core);
        return;
    }

    if (buffer_index[core] > 0) {
        printf("Core %d buffer:\n%s", core, string_buffer[core]);
        // reset
        buffer_index[core] = 0;
        string_buffer[core][0] = '\0';
    } else {
        printf("Core %d buffer is empty.\n", core);
    }
}



void initVirtualCores(){
    if(getSchedulingAlgorithm() == RMC || getSchedulingAlgorithm() == RRC || getSchedulingAlgorithm() == EDFC){
        for (size_t i = 0; i < task_count; i++)
        {
            xTaskCreatePinnedToCore(
                tasks[i].task_function,
                tasks[i].task_name,
                2048,
                &tasks[i],
                0,                    // priority
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
    for (int i = 0; i < BUFFER_SIZE_C; i++) {
        //Core 0      Core 1
        //tick,taskid,realcore,virtualcore,tick,taskid,realcore,virtualcore
        printf("###%d\n", tickCounter);
        printf("$%d,%d,%d,%d,%d,%d,%d,%d\n", gantt_buffer_0[i][0],gantt_buffer_0[i][1],gantt_buffer_0[i][2],gantt_buffer_0[i][3],
             gantt_buffer_1[i][0],gantt_buffer_1[i][1],gantt_buffer_1[i][2],gantt_buffer_1[i][3]
        );
    }
    printf("\n\n\n\n");
}

extern int core_0_buffer_full;
extern int core_1_buffer_full;

void print_task(){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while(1) {

        //if (core_0_buffer_full && core_1_buffer_full)
        //{
        //    printTaskIdsBuffer();
        //    break;
        //}
        printAndClearStringBuffer(0); // (real core ID)
        printAndClearStringBuffer(1); // (real core ID)
        printAllReadyLists();
        vTaskDelay(600 / portTICK_PERIOD_MS);
    }
    printf("Print task have been finished.\n");
    vTaskDelete(NULL); // Delete this task after printing
}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
/////////////// Modifique apenas o que estiver abaixo 

void hello_task(void *pvParameter)
{
    struct Parameters* params = (struct Parameters*)pvParameter;
    while (1) {
        char line[32]; 
        
        //addToStringBuffer("Hello: ");                                   //Adiciona string ao buffer de print
        
        snprintf(line, sizeof(line), "%lu\n", end_scheduler_timer);   //Transforma int em string
        //addToStringBuffer(line); 
        //snprintf(line, sizeof(line), "%lu\n", start_scheduler_timer);   //Transforma int em string
        //addToStringBuffer(line); 
        //snprintf(line, sizeof(line), "%lu\n", (end_scheduler_timer - start_scheduler_timer));   //Transforma int em string
        //addToStringBuffer(line);                                        //Adiciona string ao buffer de print
    }
}

void app_main(void)
{
    setSchedulingAlgorithm(EDFC);  // RMC - Rate Monotonic     // RRC - Round Robin

    xTaskCreatePinnedToCore(  //////////// Não modifique essa task
        print_task,           // Function that implements the task
        "PrintTask",          // Text name for debugging
        2048,                 // Stack size in words
        NULL,                 // Task input parameter
        tskIDLE_PRIORITY + 1, // Priority of the task
        NULL,                  // Task handle
        0                      //
    );                         ////////////// 


    task_count = 16;

    tasks[0].period = 45;
    tasks[0].task_function = hello_task;
    tasks[0].task_name = strdup("Hello Task 0");
    tasks[0].computing_time = 4;
    tasks[0].final_task = 0;
    tasks[0].task_virtual_core = 0;

    tasks[1].period = 50;
    tasks[1].task_function = hello_task;
    tasks[1].task_name = strdup("Hello Task 1");
    tasks[1].computing_time = 2;
    tasks[1].final_task = 0;
    tasks[1].task_virtual_core = 0;

    tasks[2].period = 15;
    tasks[2].task_function = hello_task;
    tasks[2].task_name = strdup("Hello Task 2");
    tasks[2].computing_time = 1;
    tasks[2].final_task = 0;
    tasks[2].task_virtual_core = 1;

    tasks[3].period = 200;
    tasks[3].task_function = hello_task;
    tasks[3].task_name = strdup("Hello Task 3");
    tasks[3].computing_time = 21;
    tasks[3].final_task = 0;
    tasks[3].task_virtual_core = 1;

    tasks[4].period = 60;
    tasks[4].task_function = hello_task;
    tasks[4].task_name = strdup("Hello Task 4");
    tasks[4].computing_time = 4;
    tasks[4].final_task = 0;
    tasks[4].task_virtual_core = 2;

    tasks[5].period = 40;
    tasks[5].task_function = hello_task;
    tasks[5].task_name = strdup("Hello Task 5");
    tasks[5].computing_time = 3;
    tasks[5].final_task = 0;
    tasks[5].task_virtual_core = 2;

    tasks[6].period = 20;
    tasks[6].task_function = hello_task;
    tasks[6].task_name = strdup("Hello Task 6");
    tasks[6].computing_time = 2;
    tasks[6].final_task = 0;
    tasks[6].task_virtual_core = 3;

    tasks[7].period = 35;
    tasks[7].task_function = hello_task;
    tasks[7].task_name = strdup("Hello Task 7");
    tasks[7].computing_time = 2;
    tasks[7].final_task = 0;
    tasks[7].task_virtual_core = 3;

    tasks[8].period = 50;
    tasks[8].task_function = hello_task;
    tasks[8].task_name = strdup("Hello Task 8");
    tasks[8].computing_time = 5;
    tasks[8].final_task = 0;
    tasks[8].task_virtual_core = 4;

    tasks[13].period = 80;
    tasks[13].task_function = hello_task;
    tasks[13].task_name = strdup("Hello Task 13");
    tasks[13].computing_time = 4;
    tasks[13].final_task = 0;
    tasks[13].task_virtual_core = 4;

    tasks[9].period = 40;
    tasks[9].task_function = hello_task;
    tasks[9].task_name = strdup("Hello Task 9");
    tasks[9].computing_time = 8;
    tasks[9].final_task = 0;
    tasks[9].task_virtual_core = 5;

    tasks[14].period = 80;
    tasks[14].task_function = hello_task;
    tasks[14].task_name = strdup("Hello Task 14");
    tasks[14].computing_time = 2;
    tasks[14].final_task = 0;
    tasks[14].task_virtual_core = 5;

    tasks[10].period = 100;
    tasks[10].task_function = hello_task;
    tasks[10].task_name = strdup("Hello Task 10");
    tasks[10].computing_time = 7;
    tasks[10].final_task = 0;
    tasks[10].task_virtual_core = 6;

    tasks[11].period = 60;
    tasks[11].task_function = hello_task;
    tasks[11].task_name = strdup("Hello Task 11");
    tasks[11].computing_time = 5;
    tasks[11].final_task = 0;
    tasks[11].task_virtual_core = 6;

    tasks[12].period = 100;
    tasks[12].task_function = hello_task;
    tasks[12].task_name = strdup("Hello Task 12");
    tasks[12].computing_time = 20;
    tasks[12].final_task = 0;
    tasks[12].task_virtual_core = 7;

    tasks[15].period = 100;
    tasks[15].task_function = hello_task;
    tasks[15].task_name = strdup("Hello Task 15");
    tasks[15].computing_time = 1;
    tasks[15].final_task = 1;
    tasks[15].task_virtual_core = 7;
    
    task_count = 16;

    initVirtualCores();
}