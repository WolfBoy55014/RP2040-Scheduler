#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <stdio.h>
#include <pico/stdlib.h>

#define MAX_TASKS 2
#define STACK_SIZE 128 // x4b
#define LOOP_TIME 10 //ms

#define DEBUG_LED 25
#define DEBUG_PRINT

#define LED_INIT gpio_init(DEBUG_LED); gpio_set_dir(DEBUG_LED, GPIO_OUT); gpio_put(DEBUG_LED, true);
#define LED_WARN gpio_put(DEBUG_LED, false)

#ifdef DEBUG_PRINT
#define PRINT_WARNING(msg) printf(msg)
#else
#define PRINT_WARNING(msg)
#endif

enum task_state {
    RUNNING,
    SUSPENDED
};

typedef struct {
    uint32_t *stack_pointer;  // Current stack pointer, also points to the top of the stack
    uint32_t stack[STACK_SIZE];
    uint32_t stack_size;      // Stack size
    uint32_t *stack_base;     // Base of stack memory
    uint32_t id;         // Task identifier
    task_state state;
} task_t;

static task_t *current_task = NULL;
static task_t *task_list[MAX_TASKS];
static int current_task_index = 0;
static int num_tasks = 0;

uint32_t initScheduler();
uint32_t startScheduler();
uint32_t addTask();

#endif