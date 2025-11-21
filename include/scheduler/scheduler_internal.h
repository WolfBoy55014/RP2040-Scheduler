//
// Created by wolfboy on 8/6/25.
//

#ifndef SCHEDULER_INTERNAL_H
#define SCHEDULER_INTERNAL_H

#include "pico/types.h"
#include "kernel_config.h"

#define CORE_NUM get_core_num() // just a macro for ease

typedef enum {
    TASK_FREE,
    TASK_RUNNING,
    TASK_READY,
    TASK_SUSPENDED,
    TASK_WAIT_US,
    TASK_YIELDING,
    TASK_ZOMBIE,
    TASK_DEAD
} task_state_t;

typedef struct {
    // --- Stack Properties ---
    uint32_t *stack_pointer;    // Current stack pointer, also points to the top of the stack
    uint32_t *stack;            // Pointer to the stack
    uint32_t stack_size;        // Stack size
    uint32_t *stack_base;       // Base of stack memory

    // --- Task Properties ---
    uint32_t id;                // Task identifier
    uint8_t priority;           // Task priority (higher is more priority)

    // --- State Properties ---
    task_state_t state;      // State of the task
    absolute_time_t resume_us;  // When the task will be done sleeping (in absolute time)

    // --- System Usage Properties ---
    uint8_t cpu_usage;          // CPU utilization (0 - 100)
    uint32_t ticks_executing;   // How many ticks this task was seen running
    uint8_t stack_usage;        // Stack utilization (0 - 100)
#ifdef OPTIMIZE_STACK_MONITORING
    uint8_t stack_recalculate_cooldown; // loops until next stack usage recalculation
#endif
} task_t;

typedef struct {
    task_t *current_task;
    uint32_t current_task_index;
    uint32_t started;
    uint32_t ticks_executing;
    uint32_t ticks_idling;
    uint8_t core_usage;
} scheduler_t;

#ifdef PROFILE_SCHEDULER
typedef struct {
    float time_total;
    float time_stack_metrics;
    float time_stack_resizing;
    float time_cpu_metrics;
    float time_scheduling;
    float time_spinlock;
} scheduler_profile_t;

extern scheduler_profile_t profile;
#endif

/* Scheduler Variables */
extern scheduler_t schedulers[CORE_COUNT];
extern uint32_t num_tasks;
extern task_t tasks[MAX_TASKS];

scheduler_t *get_scheduler();
task_t *get_current_task();

void scheduler_start_this_core();
bool scheduler_is_started();
void set_scheduler_started(bool started);

void get_next_task();
void scheduler_raise_pendsv();
int32_t refresh_systick();

/**
 * @brief Get the utilization of the scheduler belonging to a core.
 * @param core_num id of the core to get the usage from
 * @return the core usage in whole numbers 0 - 100
 */
uint8_t get_core_usage(uint8_t core_num);

#endif //SCHEDULER_INTERNAL_H
