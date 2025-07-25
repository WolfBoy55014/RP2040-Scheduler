#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pico/types.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>

#define MAX_TASKS 3
#define STACK_SIZE 128 // x4b
#define LOOP_TIME 1 // ms

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

enum task_state {
    RUNNING,
    READY,
    SUSPENDED,
    WAIT_US
};

typedef struct {
    uint32_t *stack_pointer;  // Current stack pointer, also points to the top of the stack
    uint32_t stack[STACK_SIZE];
    uint32_t stack_size;      // Stack size
    uint32_t *stack_base;     // Base of stack memory
    uint32_t id;         // Task identifier
    uint32_t priority;
    enum task_state state;
    absolute_time_t resume_us;
} task_t;

extern volatile task_t *current_task;
extern volatile task_t task_list[MAX_TASKS + 1];
extern volatile uint32_t current_task_index;
extern volatile uint32_t num_tasks;
extern volatile uint32_t scheduler_started;

uint32_t startScheduler();
uint32_t addTask(void (*task_function)(uint32_t), uint32_t id, uint32_t priority);

#endif //SCHEDULER_H