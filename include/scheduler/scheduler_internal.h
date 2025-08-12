//
// Created by wolfboy on 8/6/25.
//

#ifndef SCHEDULER_INTERNAL_H
#define SCHEDULER_INTERNAL_H

#include "pico/types.h"

#define CORE_NUM get_core_num()
#define CORE_COUNT 2

#define MAX_TASKS  8   // +2 for idle
#define STACK_SIZE 256 // x4b
#define LOOP_TIME  1 // ms

#define STACK_FILLER 0x1ABE11ED

#define LED_DEBUG_PIN 25
#define LED_WARN_PIN  5
#define LED_FATAL_PIN 6
// #define PRINT

#define LED_INIT(pin) gpio_init(pin); gpio_set_dir(pin, GPIO_OUT); gpio_put(pin, false)
#define LED_FLAG(pin) gpio_put(pin, true)
#define LED_BLINK(pin) gpio_put(pin, !gpio_get(pin));

#ifdef PRINT
#define PRINT_WARNING(msg) printf(msg)
#define PRINT_DEBUG(msg) printf(msg)
#else
#define PRINT_WARNING(msg)
#define PRINT_DEBUG(msg)
#endif

typedef enum {
    TASK_FREE,
    TASK_RUNNING,
    TASK_READY,
    TASK_SUSPENDED,
    TASK_WAIT_US,
    TASK_YIELDING,
    TASK_DEAD
} task_state_t;

typedef struct {
    uint32_t *stack_pointer;    // Current stack pointer, also points to the top of the stack
    uint32_t *stack;            // Pointer to the stack
    uint32_t stack_size;        // Stack size
    uint32_t *stack_base;       // Base of stack memory
    uint32_t id;                // Task identifier
    uint8_t priority;           // Task priority (higher is more priority)
    task_state_t state;      // State of the task
    absolute_time_t resume_us;  // When the task will be done sleeping (in absolute time)
    uint8_t cpu_usage;          // CPU utilization (0 - 100)
    uint32_t ticks_executing;   // How many ticks this task was seen running
    uint8_t stack_usage;        // Stack utilization (0 - 100)
} task_t;

typedef struct {
    task_t *current_task;
    uint32_t current_task_index;
    uint32_t started;
    uint64_t total_ticks_executing;
    uint8_t total_cpu_usage;
} scheduler_t;

/* Scheduler Variables */
extern volatile scheduler_t schedulers[CORE_COUNT];
extern volatile uint32_t num_tasks;
extern volatile task_t tasks[MAX_TASKS];

void start_scheduler_this_core();
scheduler_t *get_scheduler();
task_t *get_current_task();
bool is_scheduler_started();
void set_scheduler_started(bool started);
void get_next_task();
void raise_pendsv();

#endif //SCHEDULER_INTERNAL_H
