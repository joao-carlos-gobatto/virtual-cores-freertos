#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/timer.h>
#include <string.h>

/*Define Clock de interrup??o
 * Tabela de Interrup??o :
 * 1       ClkT = 1   segundo
 * 0.1     ClkT = 100 milissegundos
 * 0.01    Clkt = 10  milissegundos
 * 0.001   Clkt = 1   milissegundo
 * 0.0001  ClkT = 100 microssegundos
 * 0.00001 ClkT = 10  microssegundos
*/
#define ClkT 2
#define Slice 1000000 //1 segundo


#define MAX_TASKS 7      // Número máximo de tarefas
#define STACK_SIZE 2048    // Tamanho da pilha para cada tarefa
#define TIMER_GROUP TIMER_GROUP_0 // Grupo do timer
#define TIMER_INDEX TIMER_0       // Timer usado
#define TIMER_DIVIDER 80          // Divisor do clock (para 1 MHz)
#define TIMER_INTERVAL_SEC 0.5     // Intervalo em segundos (ajustável)
#define MaxNumberTask 7
#define NUM_TASKS 7
// Estruturas de contexto e tarefa
typedef struct {
    uint32_t *sp;           // Ponteiro da pilha
    uint32_t pc;            // Program Counter (PC) inicial
    uint32_t regs[12];      // Registradores a4-a15
    uint32_t stack[STACK_SIZE]; // Pilha estática da tarefa
} TaskContext_t;

typedef struct {
  TaskContext_t context;  // Contexto da tarefa
  int16_t Tid;
  const char *name;
  unsigned int Prio;
  unsigned int Time;
  unsigned short Join;
  unsigned short State;
} Task_t;

// Variáveis globais
Task_t tasks[MAX_TASKS];
int currentTask = 0;


#define MAX_NKREAD_QUEUE 5 // N?mero m?ximo de threads esperando por leitura
#define MAX_NAME_LENGTH 30
unsigned int NumberTaskAdd=-1;
volatile int TaskRunning = 0;
char myName[MAX_NAME_LENGTH];
int  SchedulerAlgorithm ;

enum Scheduler{
  RR,
  RM,
  EDF
};
enum Taskstates{
  INITIAL,
  READY,
  RUNNING,
  DEAD,
  BLOCKED
};
typedef struct 
{
    int queue[MaxNumberTask];
    int tail;
    int head;
}ReadyList;
ReadyList ready_queue;

typedef struct 
{
  short count;
  int sem_queue[MaxNumberTask], tail, header;
}sem_t;

typedef struct {
    int tid; // ID da thread esperando pela leitura
    const char *format; // Formato da entrada esperado (similar ao scanf)
    void *var; // Argumentos onde os dados ser?o armazenados
} NkReadQueueEntry;

NkReadQueueEntry nkreadQueue[MAX_NKREAD_QUEUE];
int nkreadQueueHead = 0;
int nkreadQueueTail = 0;
char serialInputBuffer[128]; // Buffer para armazenar a entrada da serial
int serialInputIndex = 0;

typedef struct
{
  int CallNumber;
  unsigned char *p0;
  unsigned char *p1;
  unsigned char *p2;
  unsigned char *p3;
}Parameters; 

Parameters kernelargs ;


typedef struct {
  int16_t Tid;
  const char *name;
  unsigned int Prio;
  unsigned int Time;
  unsigned short Join;
  unsigned short State;
 // uint8_t Stack[SizeTaskStack]; // Vetor de pilha
  uint8_t* P; // Ponteiro de pilha
} TaskDescriptor;
TaskDescriptor Descriptors[MaxNumberTask]; // Array de descritores de tarefas
/* 
*
*Servicos do kernel
*
*/
enum sys_temCall{
  TASKCREATE,
  SEM_WAIT,
  SEM_POST,
  SEM_INIT,
  WRITELCDN,
  WRITELCDS,
  EXITTASK,
  SLEEP,
  MSLEEP,
  USLEEP,
  LIGALED,
  DESLIGALED,
  START, 
  TASKJOIN,
  SETMYNAME,
  GETMYNAME,
  NKPRINT,
  GETMYNUMBER,
  NKREAD,
};

/*
*
* Rotinas do kernel
*
*
*/


// Protótipos
void saveContext(TaskContext_t *taskContext);
void restoreContext(TaskContext_t *taskContext);
void scheduler();
void systemContext();
void initTask(int *taskIndex, void (*taskFunction)(void), unsigned int priority);
void kernel();
void initTimer();
void IRAM_ATTR onTimer(void *arg);
void sys_taskcreate(int *tid, void (*taskFunction)(void), int *priority);
void task0();
void task1();
void task2();
void task3();
void task4();
void task5();
void task6();
void testeFunc();

/*
* Passa a executar a rotina do kernel com interruo??es desabiitadas
*
*/
void callsvc(Parameters *args)
{
    portDISABLE_INTERRUPTS();
    kernelargs = *args ;
    kernel() ;
    portENABLE_INTERRUPTS();
}
/*
*
*
*/

/*
*   
* Algoritmo Bubble Sort para a reordena??o da Ready List
* O crit?rio de ordena??o ? a prioridade (Prio) definida para a task
* Menor valor n?merico indica maior prioridade
*
*/
void sortReadyList() {
    for (int i = 0; i < ready_queue.head - 1; i++) {
        for (int j = 0; j < ready_queue.head - i - 1; j++) {
            if (Descriptors[ready_queue.queue[j]].Prio > Descriptors[ready_queue.queue[j + 1]].Prio) {
                int temp = ready_queue.queue[j];
                ready_queue.queue[j] = ready_queue.queue[j + 1];
                ready_queue.queue[j + 1] = temp;
            }
        }
    }
}

/*
* A inser??o da task ? inicialmente realizada no final da Ready List
* ? chamada sortReadyList() para reordena??o por prioridade
*/
void InsertReadyList(int id) {
    ready_queue.queue[ready_queue.head] = id;
    ready_queue.head++;
    sortReadyList();
  }

void wakeUP() //acorda a task bloqueada a espera de passagem de tempo
{
  int i=1;
  for(i=1;i<=NUM_TASKS; i++)
  {
    //sleep
    if(Descriptors[i].Time>0)
    {
      Descriptors[i].Time--;
      if(Descriptors[i].Time <= 0 && Descriptors[i].State == BLOCKED)
      {
        Descriptors[i].State = READY;
        InsertReadyList(i) ; //tempo de espera se esgotou
      }
    }
  }
}

//Escalonador

/*
* Imprime o conte?do da Ready List
* Usada para verifica??o durante testes
*/
 void printReadyList() {
    printf("\nReady list tasks: ");
    for (int i = 0; i < ready_queue.head; i++) {
        printf("\n Index:");
        printf("%d\n", ready_queue.queue[i]);
    }
    printf("\n ");
}


/*
*   
* Se a task atual n?o ? Idle (TaskRunning != 0), a task ? removida da Ready List
* Caso n?o esteja bloqueada, ela ? reinserida no final da Ready List
* A remo??o ? feita com o deslocamento para a esquerda 
* Atualiza TaskRunning com a primeira task da Ready List (TaskRunning = ready_queue.queue[0])
* Se a Ready List estiver vazia, TaskRunning ser? 0 (Idle) 
*
*/
void switchTask() {    
 // saveContext(&Descriptors[TaskRunning]);

  if (TaskRunning != 0){
    for (int i = 0; i < ready_queue.head - 1; i++) {
      ready_queue.queue[i] = ready_queue.queue[i + 1];
    }
    ready_queue.head--;
    if (Descriptors[TaskRunning].State != BLOCKED)  {   
      InsertReadyList(TaskRunning);   
    }
  }
  
  if (ready_queue.head > 0){
    TaskRunning = ready_queue.queue[0];
  } else {
    TaskRunning = 0;
  }
  
  Descriptors[TaskRunning].State = RUNNING;
  //restoreContext(&Descriptors[TaskRunning]);
}


/*
*
* Idle Process
*
*/
void idle() {
     while (1) {
     } ;
}


/*
*
* Sys Call
*
*/
void sys_taskcreate(int *tid, void (*taskFunction)(void), int *priority) {
/*
    NumberTaskAdd++ ;
    *tid = NumberTaskAdd;
    Descriptors[NumberTaskAdd].Tid=*tid;
    Descriptors[NumberTaskAdd].State=READY;
    Descriptors[NumberTaskAdd].Join=0;
    Descriptors[NumberTaskAdd].Time=0 ;
    Descriptors[NumberTaskAdd].Prio=priority;
    uint8_t* stack = Descriptors[*tid].Stack + SizeTaskStack - 1;
    Descriptors[*tid].P = stack;

    *(stack--) = ((uint16_t)taskFunction) & 0xFF; // PC low byte
    *(stack--) = ((uint16_t)taskFunction >> 8) & 0xFF; // PC high byte
    *(stack--) = 0x00; // R0
    *(stack--) = 0x80; // SREG with global interrupts enabled

    for (int i = 1; i < 32; i++) {
        *(stack--) = i; // Initialize all other registers with their number
    }

    Descriptors[*tid].P = stack;
  */
}



void sys_start(int scheduler) {
    int i;
    SchedulerAlgorithm = scheduler;
    switch (SchedulerAlgorithm) {
        case RR:
            for (i = 1; i <= NumberTaskAdd; i++) {
                InsertReadyList(i);
            }
            break;
        default:
            break;
    }
}

void sys_getmynumber(int *number)
{
  *number=Descriptors[TaskRunning].Tid ;
}

void sys_ligaled()
{
 // PORTB = PORTB | 0x20;
}

void sys_desligaled()
{
 // PORTB = PORTB & 0xDF;
}

void sys_setmyname(const char *name)
{
  Descriptors[TaskRunning].name=name;
}

void sys_getmyname(char *name)
{
  strcpy(name, Descriptors[TaskRunning].name);
}

void sys_semwait(sem_t *semaforo)
{   
    semaforo->count--;
    if(semaforo->count < 0)
    {
     // printf("\nSemWait: ");
      semaforo->sem_queue[semaforo->tail] = currentTask;
      tasks[currentTask].State = BLOCKED ;
      semaforo->tail++;
      if(semaforo->tail == MaxNumberTask-1) semaforo->tail = 0;
      //switchTask();
      systemContext() ;

    
    }
}

void sys_sempost(sem_t *semaforo)
{
     semaforo->count++;
     if(semaforo->count <= 0)
     {
       tasks[semaforo->sem_queue[semaforo->header]].State = READY;
      // InsertReadyList(semaforo->sem_queue[semaforo->header]);
       semaforo->header++;
       if(semaforo->header == MaxNumberTask-1) semaforo->header = 0;
       systemContext() ;

     }
}

void sys_seminit(sem_t *semaforo, int ValorInicial)
{
  semaforo->count = ValorInicial;
  semaforo->header = 0;
  semaforo->tail = 0;
}

void syssleep(unsigned int segundo)
{
  //Descriptors[TaskRunning].Time = segundo/ClkT;
  Descriptors[TaskRunning].Time = (segundo*1000000)/Slice ;
  if(Descriptors[TaskRunning].Time > 0)
  {
    Descriptors[TaskRunning].State = BLOCKED;
    switchTask();
    
    //select() ;
  }
}
void sysmsleep(unsigned int mili)
{
  Descriptors[TaskRunning].Time = (mili/ClkT)/1000;
  if(Descriptors[TaskRunning].Time > 0)
  {
    Descriptors[TaskRunning].State = BLOCKED;
    switchTask();
  }
}

void sysusleep(unsigned int micro)
{
  Descriptors[TaskRunning].Time = (micro/ClkT)/1000000;
  if(Descriptors[TaskRunning].Time > 0)
  {
    Descriptors[TaskRunning].State = BLOCKED;
    switchTask();
  }
}

/*
*  calcularPrecisao( float valor) chamada pela sys_nkprint
*/
static inline int calcularPrecisao( float valor)
{
  int PRECISAO_FLOAT_ARDUINO = 6;
  int precisao = 0;
  int valorInteiro = (int)valor;
  while(valorInteiro > 0)
  {
    valorInteiro = valorInteiro / 10;
    precisao++;
  }
  return PRECISAO_FLOAT_ARDUINO - precisao;
}
void sys_nkprint(char *fmt,void *number)
{
  int *auxint;
  float *auxfloat = NULL;
  char *auxchar;
  int size=0;
  int accuracy=1;
  int precisao ;
  
  while (*fmt)
  {
    switch(*fmt)
      {
        case '%':
          fmt++;
          switch(*fmt)
            {
              case '%':
                printf("%c\n", *fmt);
                //Serial.flush();  // Aguarda até que todos os dados sejam transmitidos
                break;
              case 'c':
                printf("%s\n",(char *)number);
                //Serial.flush();  // Aguarda até que todos os dados sejam transmitidos
                break;
              case 's':
                sys_nkprint((char *)number,0);
                break;
              case 'd':
                auxint= (int *)number;
                printf("%c\n",*auxint);
                //Serial.flush();  // Aguarda até que todos os dados sejam transmitidos
                break;
              case 'f':
                *auxfloat = *( (float*) number );
                 precisao = calcularPrecisao(*auxfloat);
                printf("%f\t%d\n",*auxfloat, precisao);
                //Serial.flush();  // Aguarda até que todos os dados sejam transmitidos
                break;
              // case '.':
              //   fmt++;
              //   while(*fmt != 'f')
              //   {
              //     size*=accuracy;
              //     size+= (*fmt - '0');
              //     accuracy*=10;
              //     fmt++;
              //   }
              //   auxfloat=number;
              //   // printfloat(*auxfloat,size);
              //   break;
              // case 'x':
              //   auxint=number;
              //   // printhexL(*auxint);
              //   break;
              // case 'X':
              //   auxint=number;
              //   // printhexU(*auxint);
              //   break;
              // case 'b':
              //   fmt++;
              //   switch(*fmt)
              //   {
              //     case 'b':
              //       size=8;
              //       break;
              //     case 'w':
              //       size=16;
              //       break;
              //     case 'd':
              //       size=32;
              //       break;
              //     default:
              //       fmt -= 2;
              //       size = 32;
              //       break;
              //   }
              //   auxint=number;
              //   // printbinary(*auxint, size);
              //   break;
              default:
                break;
            }
        break;
        case '\\':
          fmt++;
          if (*fmt == 'n')
          {
            printf("\n\n");
            //Serial.flush();  // Aguarda até que todos os dados sejam transmitidos
          }
          else
          {
            printf("\n\\");
            //Serial.flush();  // Aguarda até que todos os dados sejam transmitidos
          }
        break;
        default:
          printf("%c\n", *fmt);
          //Serial.flush();  // Aguarda até que todos os dados sejam transmitidos
        break;
      }
    fmt++;
  }
}

void sys_taskexit(void)
{
  Descriptors[TaskRunning].State=BLOCKED;
  switchTask();
}
void enqueueNkRead(int tid, const char *format, void *var) {
    nkreadQueue[nkreadQueueTail].tid = tid;
    nkreadQueue[nkreadQueueTail].format = format;
    nkreadQueue[nkreadQueueTail].var = var;
    nkreadQueueTail = (nkreadQueueTail + 1) % MAX_NKREAD_QUEUE;
}

NkReadQueueEntry dequeueNkRead() {
    NkReadQueueEntry entry = nkreadQueue[nkreadQueueHead];
    nkreadQueueHead = (nkreadQueueHead + 1) % MAX_NKREAD_QUEUE;
    return entry;
}

void sys_nkread(const char *format, void *var) {
  // Adicionar a thread atual na fila de leitura
  enqueueNkRead(Descriptors[TaskRunning].Tid, format, var);
  // Bloquear a thread atual
  Descriptors[TaskRunning].State = BLOCKED;
  switchTask();
}

float stringToFloat(const char* str) {
    float result = 0.0;
    float factor = 1.0;

    if (*str == '-') {
        str++;
        factor = -1.0;
    }

    // Parte inteira
    for (; *str >= '0' && *str <= '9'; str++) {
        result = result * 10.0 + (*str - '0');
    }

    // Parte fracion?ria
    if (*str == '.') {
        float fraction = 0.1;
        str++;
        for (; *str >= '0' && *str <= '9'; str++) {
            result += (*str - '0') * fraction;
            fraction *= 0.1;
        }
    }

    return result * factor;
}

void serialEvent() {
    // while (Serial.available()) {
    //     char c = Serial.read();
    //     if (c == '\n') {
    //         serialInputBuffer[serialInputIndex] = '\0';  // Termina a string
    //         serialInputIndex = 0;  // Reinicia o ?ndice

    //         // Desbloquear a thread que est? esperando por entrada
    //         if (nkreadQueueHead != nkreadQueueTail) {
    //             NkReadQueueEntry entry = dequeueNkRead();
    //             if (strcmp(entry.format, "%f") == 0) {
    //                 // Para float, usar nossa fun??o auxiliar
    //                 *(float *)(entry.var) = stringToFloat(serialInputBuffer);
    //             } else {
    //                 // Interpretar a entrada de acordo com o formato fornecido
    //                 sscanf(serialInputBuffer, entry.format, entry.var);
    //             }
    //             Descriptors[entry.tid].State = READY;
    //         }
    //     } else {
    //         if (serialInputIndex < 127) {
    //             serialInputBuffer[serialInputIndex++] = c;
    //         }
    //     }
    // }
}

/*
*
* User Call
*
*/
void taskcreate(int *ID,void (*funcao)(), int *Priority) //parametros armazenados em R0 e R1 na chamada
{
  Parameters arg;
  arg.CallNumber=TASKCREATE;
  arg.p0=(unsigned char *)ID;
  arg.p1=(unsigned char *)funcao;
  arg.p2=(unsigned char *)Priority;
  callsvc(&arg);
}
void start(int scheduler)
{
  Parameters arg;
  arg.CallNumber=START;
  arg.p0=(unsigned char *)scheduler;
  callsvc(&arg);
}
void semwait(sem_t *semaforo)
{
  Parameters arg;
  arg.CallNumber=SEM_WAIT;
  arg.p0=(unsigned char *)semaforo;
  callsvc(&arg);
}

void sempost(sem_t *semaforo)
{
  Parameters arg;
  arg.CallNumber=SEM_POST;
  arg.p0=(unsigned char *)semaforo;
  callsvc(&arg);
}

void seminit(sem_t *semaforo, int ValorInicial)
{
  Parameters arg;
  arg.CallNumber=SEM_INIT;
  arg.p0=(unsigned char *)semaforo;
  arg.p1=(unsigned char *)ValorInicial;
  callsvc(&arg);
}
void setmyname(const char *name)
{
  Parameters arg;
  arg.CallNumber=SETMYNAME;
  arg.p0=(unsigned char *)name;
  callsvc(&arg);
}
void getmynumber(int *number)
{
  Parameters arg;
  arg.CallNumber=GETMYNUMBER;
  arg.p0=(unsigned char *)number;
  callsvc(&arg);
}
void getmyname(char *name)
{
  Parameters arg;
  arg.CallNumber=GETMYNAME;
  arg.p0=(unsigned char *)name;
  callsvc(&arg);
}
void user_sleep(int time)
{
  Parameters arg;
  arg.CallNumber=SLEEP;
  arg.p0=(unsigned char *)time;
  callsvc(&arg);
}

void user_msleep(int time)
{  
  Parameters arg;
  arg.CallNumber=SLEEP;
  arg.CallNumber=MSLEEP;
  arg.p0=(unsigned char *)time;
  callsvc(&arg);
}

void user_usleep(int time)
{
  Parameters arg;
  arg.CallNumber=USLEEP;
  arg.p0=(unsigned char *)time;
  callsvc(&arg);
}
void taskexit(void)
{
  Parameters arg;
  arg.CallNumber=EXITTASK;
  callsvc(&arg);
}

void ligaled(void)
{
  Parameters arg;
  arg.CallNumber=LIGALED;
  callsvc(&arg);
}

void desligaled(void)
{
  Parameters arg;
  arg.CallNumber=DESLIGALED;
  callsvc(&arg);
}

void nkprint(char *fmt,void *number)
{
  Parameters arg;
  arg.CallNumber=NKPRINT;
  arg.p0=(unsigned char *)fmt;
  arg.p1=(unsigned char *)number;
  callsvc(&arg);
}
void nkread(const char *format, void *var) {
    Parameters arg;
    arg.CallNumber = NKREAD;
    arg.p0 = (unsigned char *)format;
    arg.p1 = (unsigned char *)var;
    callsvc(&arg);
}

int t0 ; 
int t1 ; 
int t2 ; 
int t3 ; 
int t4 ; 
int t5 ; 
int t6 ; 

sem_t s0 ;

// Função para inicializar uma tarefa
void initTask(int *taskIndex, void (*taskFunction)(void), unsigned int priority) {
   // if (taskIndex >= 0 && taskIndex < MAX_TASKS) {
        NumberTaskAdd++ ;
        *taskIndex = NumberTaskAdd;
        tasks[NumberTaskAdd].Tid=*taskIndex;
        tasks[NumberTaskAdd].State=READY;
        tasks[NumberTaskAdd].Join=0;
        tasks[NumberTaskAdd].Time=0 ;
        tasks[NumberTaskAdd].Prio=priority;
        tasks[NumberTaskAdd].context.sp = (uint32_t*)((uintptr_t)&tasks[NumberTaskAdd].context.stack[STACK_SIZE - 1] & ~0x3);
        tasks[NumberTaskAdd].context.pc = (uint32_t)taskFunction;
        for (int i = 0; i < 12; i++) {
            tasks[NumberTaskAdd].context.regs[i] = 0;
        }
        printf("\nTarefa ");
        printf("%d", NumberTaskAdd);
        printf("\n inicializada.");
   // }
}

// Salva o contexto da tarefa atual
void saveContext(TaskContext_t *taskContext) {
    asm volatile(
        "mov %0, sp\n" // Salva o Stack Pointer (SP)
        : "=r"(taskContext->sp)
        :
        : "memory"
    );

    asm volatile(
        "s32i a4, %0, 0\n"
        "s32i a5, %0, 4\n"
        "s32i a6, %0, 8\n"
        "s32i a7, %0, 12\n"
        "s32i a8, %0, 16\n"
        "s32i a9, %0, 20\n"
        "s32i a10, %0, 24\n"
        "s32i a11, %0, 28\n"
        "s32i a12, %0, 32\n"
        "s32i a13, %0, 36\n"
        "s32i a14, %0, 40\n"
        "s32i a15, %0, 44\n"
        :
        : "r"(taskContext->regs)
    );
}

// Restaura o contexto da próxima tarefa
void restoreContext(TaskContext_t *taskContext) {
    asm volatile(
        "mov sp, %0\n" // Restaura o Stack Pointer (SP)
        :
        : "r"(taskContext->sp)
    );

    asm volatile(
        "l32i a4, %0, 0\n"
        "l32i a5, %0, 4\n"
        "l32i a6, %0, 8\n"
        "l32i a7, %0, 12\n"
        "l32i a8, %0, 16\n"
        "l32i a9, %0, 20\n"
        "l32i a10, %0, 24\n"
        "l32i a11, %0, 28\n"
        "l32i a12, %0, 32\n"
        "l32i a13, %0, 36\n"
        "l32i a14, %0, 40\n"
        "l32i a15, %0, 44\n"
        :
        : "r"(taskContext->regs)
    );

    uint32_t pc = taskContext->pc;
       // initTimer();
        portENABLE_INTERRUPTS();
    asm volatile(
        "mov a0, %0\n"
        "jx a0\n" // Salta para o endereço armazenado no PC restaurado
        :
        : "r"(pc)
    );
}

// Função do timer para alternar tarefas
void IRAM_ATTR onTimer(void *arg) {
    // Limpa a interrupção
    timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_INDEX);

    // Reinicia o alarme
    timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER_INDEX);

    // Define a flag para alternar tarefas
    //taskSwitch = true;
       systemContext() ;
      // scheduler() ;
}

// Inicializa o timer
void initTimer() {
    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider = TIMER_DIVIDER};

    timer_init(TIMER_GROUP, TIMER_INDEX, &config);

    timer_set_counter_value(TIMER_GROUP, TIMER_INDEX, 0x00000000ULL);
    timer_set_alarm_value(TIMER_GROUP, TIMER_INDEX, TIMER_INTERVAL_SEC * (80000000 / TIMER_DIVIDER));
    timer_enable_intr(TIMER_GROUP, TIMER_INDEX);

    timer_isr_register(TIMER_GROUP, TIMER_INDEX, onTimer, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP, TIMER_INDEX);
}

// Escalonador simples
void systemContext() {
    portDISABLE_INTERRUPTS();
    //if (taskSwitch) {
    //    taskSwitch = false;
        scheduler();
   // }
       //    portENABLE_INTERRUPTS();
}


// Alterna entre tarefas
void scheduler() {
      //  portDISABLE_INTERRUPTS
    saveContext(&tasks[currentTask].context);
    currentTask = (currentTask + 1) % MAX_TASKS;
    while (tasks[currentTask].State == BLOCKED)
        currentTask = (currentTask + 1) % MAX_TASKS;
   // portENABLE_INTERRUPTS();
    restoreContext(&tasks[currentTask].context);
}

void kernel() {
    switch(kernelargs.CallNumber){
    case TASKCREATE: // OK
      sys_taskcreate((int *)kernelargs.p0,(void(*)())kernelargs.p1,(int *)kernelargs.p2);
      break;
    case SEM_WAIT: // OK
     // printf("\nSEMWAIT: ") ;
      sys_semwait((sem_t *)kernelargs.p0);
      break;
    case SEM_POST: // OK
      sys_sempost((sem_t *)kernelargs.p0);
      break;
    case SEM_INIT: // OK
      //printf("\nSEMINIT: ") ;
      sys_seminit((sem_t *)kernelargs.p0,(int)kernelargs.p1);
      break;
    case WRITELCDN: // NAO TEREMOS
      // LCDcomando((int)arg->p1);
      // LCDnum((int)arg->p0);
      break;
    case WRITELCDS: // NAO TEREMOS
      // LCDcomando((int)arg->p1);
      // LCDputs((char*)arg->p0);
      break;
    case EXITTASK: // OK
      sys_taskexit();
      break;
    case SLEEP: 
      syssleep((int)kernelargs.p0);
      break;
    case MSLEEP: 
      sysmsleep((int)kernelargs.p0);
      break;
    case USLEEP: 
      sysusleep((int)kernelargs.p0);
      break;
    case LIGALED: // OK
      sys_ligaled();
      break;
    case DESLIGALED: // OK
      sys_desligaled();
      break;
    case START: 
      sys_start((int)kernelargs.p0);
      break;
    case TASKJOIN: 
     // sys_taskjoin((int)arg->p0);
      break;
    case SETMYNAME: // OK
      sys_setmyname((const char *)kernelargs.p0);
      break;
    case GETMYNAME: // OK
      sys_getmyname((char *)kernelargs.p0);
      break;  
    case NKPRINT: // OK
       sys_nkprint((char *)kernelargs.p0,(void *)kernelargs.p1);
       break;
    case GETMYNUMBER: 
       sys_getmynumber((int *)kernelargs.p0);
       break;
    case NKREAD: 
       sys_nkread((char *)kernelargs.p0,(void *)kernelargs.p1);
       break;
    default:
       break;
  }
}




// Tarefas
void task0() {
   static  int32_t LocalVar0 = 0;
   seminit (&s0, 0) ;
   nkprint ("task0: Acordei...\n", 0);

    while (1) {
        for (int i = 0; i<500000; i++)  LocalVar0++;
        nkprint ("Task 0 LocalVar0: ", 0);
       // printf("\nTask 0 LocalVar0: ");
      //printf("%ld\n", LocalVar0);
       nkprint ("%ld\n", &LocalVar0);       
       semwait(&s0) ;

     //   delay(10);       

    }
}      


void task1() {
    static int32_t LocalVar1 = 100;
    while (1) {
        for (int i = 0; i<500000; i++)  LocalVar1++;
        nkprint ("Task 1 LocalVar1: ", 0);
        nkprint ("%ld\n", &LocalVar1);
       // printf("\nTask 1 LocalVar1: ");
       // printf("%ld\n", LocalVar1);
       // delay(10);
    }
}

void task2() {
    static int32_t LocalVar2 = 200;
    while (1) {
        for (int i = 0; i<500000; i++) LocalVar2++;
        printf("\nTask 2 LocalVar2: ");
        printf("%ld\n", LocalVar2);
      //  testeFunc() ;
        //delay(10);
    }
}

void task3() {
    static int32_t LocalVar3 = 300;
   // testeFunc() ;
    while (1) {
   //     printf("\nLocalVar3: ");
      
      for (int i = 0; i<500000; i++) LocalVar3++;
      printf("\nTask 3 LocalVar3: ");
      printf("%ld\n", LocalVar3);
      //delay(10);
    }
}  


void task4() {
   static  int32_t LocalVar4 = 0;
    while (1) {
        for (int i = 0; i<500000; i++)  LocalVar4++;
        printf("\nTask 4 LocalVar4: ");
        printf("%ld\n", LocalVar4);
       // delay(10);       

    }
}

void task5 () {
   static  int32_t LocalVar5 = 0;
    while (1) {
        for (int i = 0; i<500000; i++)  LocalVar5++;
        printf("\nTask 5 LocalVar5: ");
        printf("%ld\n", LocalVar5);
       // delay(10);       

    }
}
void task6 () {
   static  int32_t LocalVar6 = 0;
    while (1) {
        for (int i = 0; i<500000; i++) LocalVar6++; 
          printf("\nTask 6 LocalVar6: ");
          printf("%ld\n", LocalVar6) ;
        //  testeFunc() ;
         sempost(&s0) ;
         // delay(10);    
    }
}

void testeFunc()  {

  static int32_t testeVar ;

      for (int i = 0; i<50; i++) { 
        testeVar++;
        printf("\nTeste  testeVar: ");
        printf("%ld\n",testeVar);
        vTaskDelay(pdMS_TO_TICKS(10));
        // delay(10);     
    }
}

void app_main(void) {
    // Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // delay(1000);
    printf("\nIniciando sistema...");

    // Inicializa as tarefas
    initTask(&t0, task0, 1);
    initTask(&t1, task1, 1);
    initTask(&t2, task2, 1);
    initTask(&t3, task3, 1);
    initTask(&t4, task4, 1);
    initTask(&t5, task5, 1);
    initTask(&t6, task6, 1);


    // Inicializa o timer
    initTimer();

    // Inicia a primeira tarefa
    restoreContext(&tasks[currentTask].context);
}