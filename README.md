# virtual-cores-freertos
Versão da ESP-IDF utilizada: 5.3.3
Simulação de cores virtuais para FreeRTOS na ESP32. Experimentação de algoritmos de escalonamento multicore

A localização da função xTaskIncrementTick está localizada em /freertos/FreeRTOS-Kernel/tasks.c

Em FreeRTOS-Kernel/portable/xtensa/include/freertos/portmacro.h existem algumas implementações em assembly de salvamento de contexto e liberação de núcleo por uma task. Essas macros são chamadas em C das seguintes formas:

/**
 * @brief Perform a solicited context switch
 *
 * - Defined in portasm.S
 *
 * @note [refactor-todo] The rest of ESP-IDF should call taskYield() instead
 */
void vPortYield( void );
Essa função é usada como taskYield() no arquivo tasks.c

/**
 * @brief Yields the other core
 *
 * - Send an interrupt to another core in order to make the task running on it yield for a higher-priority task.
 * - Can be used to yield current core as well
 *
 * @note [refactor-todo] Put this into private macros as its only called from task.c and is not public API
 * @param coreid ID of core to yield
 */
void vPortYieldOtherCore(BaseType_t coreid);
Essa função é usada como taskYIELD_CORE no arquivo tasks.c

Funções notáveis no freertos_tasks_c_additions
xTaskGetCurrentTaskHandleForCore

Funções notáveis no tasks.c
pxCurrentTCBs -> Array das tasks atuais nos cores (Só tem duas posições, o taskhandle da task no core 0 e no core 1)
xTaskGetSchedulerState

FreeRTOSConfig.h
Free

Como é que o FreeRTOS sabe quantos cores tem?
Como é que o FreeRTOS joga a task pro core?
Como é que o FreeRTOS gera o tick? de onde ele vem?

Fazer o SELECT de algoritmo de escalonamento.
Como disparar as Tasks.
Como tratar a interrupção do Timer.