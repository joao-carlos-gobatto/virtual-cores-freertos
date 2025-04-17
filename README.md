# virtual-cores-freertos
Simulação de cores virtuais para FreeRTOS na ESP32. Experimentação de algoritmos de escalonamento multicore

A localização da função xTaskIncrementTick está localizada em /freertos/FreeRTOS-Kernel/tasks.c

FreeRTOSConfig.h
Free

Como é que o FreeRTOS sabe quantos cores tem?
Como é que o FreeRTOS joga a task pro core?
Como é que o FreeRTOS gera o tick? de onde ele vem?