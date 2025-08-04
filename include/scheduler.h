#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pico/types.h>

#define MAX_TASKS 8 // +1 for idle
#define STACK_SIZE 128 // x4b
#define LOOP_TIME 0.1 // ms

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
    TASK_RUNNING,
    TASK_READY,
    TASK_SUSPENDED,
    TASK_WAIT_US,
    TASK_YIELDING
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

/* Scheduler Variables */
extern volatile uint32_t num_tasks;
extern volatile task_t *current_task;
extern volatile uint32_t current_task_index;
extern volatile task_t task_list[MAX_TASKS];
extern volatile uint32_t scheduler_started;

/* CPU Usage Variables */
extern uint64_t total_ticks_executing;
extern uint8_t total_cpu_usage;

/* Basic Scheduler Functions */
uint32_t start_scheduler();
uint32_t add_task(void (*task_function)(uint32_t), uint32_t id, uint8_t priority) ;

#endif //SCHEDULER_H