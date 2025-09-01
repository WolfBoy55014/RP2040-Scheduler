#include <stdlib.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "hardware/sync.h"
#include "hardware/structs/systick.h"
#include "pico/multicore.h"

#include "scheduler_internal.h"
#include "scheduler.h"

#include <string.h>

#include "spinlock_internal.h"
#include "kernel_config.h"

/* Scheduler Variables */
volatile scheduler_t schedulers[CORE_COUNT];
volatile uint32_t num_tasks;
volatile task_t tasks[MAX_TASKS];
#ifdef PROFILE_SCHEDULER
scheduler_profile_t profile;
#endif

/* Public Assembly Functions */
extern void set_spsel(uint32_t control);

scheduler_t *get_scheduler() {
    return &schedulers[CORE_NUM];
}

task_t *get_current_task() {
    scheduler_t *scheduler = get_scheduler();
    return scheduler->current_task;
}

bool is_scheduler_started() {
    return get_scheduler()->started;
}

void set_scheduler_started(bool started) {
    get_scheduler()->started = started;
}

void calculate_cpu_usage() {
    const uint32_t saved_irq = scheduler_spin_lock();
#ifdef PROFILE_SCHEDULER
    uint32_t start_time = time_us_32();
#endif

    uint32_t total_ticks_executing = 0;
    uint32_t total_ticks_idling = 0;

    // calculate core usage
    for (int s = 0; s < NUM_CORES; s++) {
        scheduler_t *scheduler = &schedulers[s];

        uint32_t ticks_executing = scheduler->ticks_executing;
        uint32_t ticks_idling = scheduler->ticks_idling;

        total_ticks_executing += ticks_executing;
        total_ticks_idling += ticks_idling;

        scheduler->core_usage = ((ticks_executing - ticks_idling) * 100) / ticks_executing;
        scheduler->ticks_idling = 0;
        scheduler->ticks_executing = 0;
    }

    // calculate task cpu usage
    for (int t = 0; t < MAX_TASKS; t++) {
        task_t *task = &tasks[t];

        if (task->stack == TASK_FREE) {
            continue;
        }

        task->cpu_usage = (task->ticks_executing * 100) / total_ticks_executing;
        task->ticks_executing = 0;
    }
#ifdef PROFILE_SCHEDULER
    profile.time_cpu_metrics += (time_us_32() - start_time) / 1000.0f;
#endif
    scheduler_spin_unlock(saved_irq);
}

uint8_t get_core_usage(const uint8_t core_num) {
    uint32_t saved_irq = scheduler_spin_lock();

    if (core_num >= NUM_CORES) {
        return 0; // it doesn't exist, thus it has no usage
    }

    scheduler_t *scheduler = &schedulers[core_num];
    const uint8_t usage = scheduler->core_usage;

    scheduler_spin_unlock(saved_irq);

    return usage;
}

uint32_t resize_stack(task_t *task, uint32_t new_size) {
#ifdef DYNAMIC_STACK
#if defined(PROFILE_SCHEDULER) || defined(PRINT)
    uint32_t start_time = time_us_32();
#endif

    if (new_size > MAX_STACK_SIZE) {
        new_size = MAX_STACK_SIZE;
    }

    if (new_size < MIN_STACK_SIZE) {
        new_size = MIN_STACK_SIZE;
    }

    uint32_t stack_pointer_offset = task->stack_base - task->stack_pointer;
    uint32_t old_size = task->stack_size;

    uint32_t *reallocated_stack = NULL;
    reallocated_stack = realloc(task->stack, new_size * sizeof(uint32_t));

    if (reallocated_stack == NULL) {
        return old_size;                // if its null, that means there was no more room
    }

    task->stack = reallocated_stack;
    task->stack_size = new_size;

    if (new_size > old_size) {
        const uint32_t additional_space = new_size - old_size;

        memmove(&task->stack[additional_space], &task->stack[0], old_size * sizeof(uint32_t));

        // fill the rest of the new stack with the filler
        for (uint32_t i = 0; i < additional_space; i++) {
            task->stack[i] = STACK_FILLER;
        }

        // dump the stack for debug
        // for (int i = 0; i < new_size; i++) {
        //     printf("%X\n", task->stack[i]);
        // }
    }

    task->stack_base = task->stack + task->stack_size - 1;
    task->stack_pointer = task->stack_base - stack_pointer_offset;

#ifdef PRINT
    printf("\nResizing stack took: %llu us\n", time_us_64() - start_time);
#endif
#ifdef PROFILE_SCHEDULER
    profile.time_stack_resizing += (time_us_32() - start_time) / 1000.0f;
#endif
    return new_size;

#else
    return task->stack_size;
#endif
}

bool find_and_resolve_stack_overflow(task_t *task) {
    uint32_t *check_point = task->stack + STACK_OVERFLOW_THRESHOLD - 1;

    if (*check_point != STACK_FILLER) {
#ifdef DYNAMIC_STACK
        if (task->stack_size < MAX_STACK_SIZE) {
            uint32_t old_size = task->stack_size;
            uint32_t new_size = resize_stack(task, task->stack_size + STACK_STEP_SIZE);
            if (new_size > old_size) {
                return true;
            }
        }
#endif

        task->state = TASK_SUSPENDED;
        return false;
    }

    return true;
}

void calculate_stack_usage() {
    const uint32_t saved_irq = scheduler_spin_lock();
#if defined(PROFILE_SCHEDULER) || defined(PRINT)
    uint32_t start_time = time_us_32();
#endif

    for (int t = 0; t < MAX_TASKS; t++) {
        task_t *task = &tasks[t];

        if (task->state == TASK_FREE || task->state == TASK_RUNNING) {
            continue;
        }

        // if task is dead, remove it
        if (task->state == TASK_DEAD) {
            free(task->stack);
            task->state = TASK_FREE;
            continue;
        }

#ifdef OPTIMIZE_STACK_MONITORING
        if (task->state == TASK_SUSPENDED) {
            continue;
        }

        if (task->stack_recalculate_cooldown > 0) {
            task->stack_recalculate_cooldown--;
            continue;
        }
#endif

        uint32_t total_stack = task->stack_size;
        uint32_t stack_unused = 0;

        for (int i = 0; i < total_stack; i++) {
            if (task->stack[i] == STACK_FILLER) {
                stack_unused++;
            } else {
                break;  // we started from the side that will be touched last
                        // so if this word was used so will all the rest
            }
        }

        uint32_t stack_used = total_stack - stack_unused;

        task->stack_usage = (stack_used * 100) / total_stack;

#ifdef OPTIMIZE_STACK_MONITORING
        task->stack_recalculate_cooldown = OPTIMIZE_STACK_MONITORING_FACTOR * (stack_unused / STACK_OVERFLOW_THRESHOLD);
        if (task->state == TASK_WAIT_US) {
            task->stack_recalculate_cooldown++;
        }
#endif

        if (stack_unused < STACK_OVERFLOW_THRESHOLD) {
#ifdef DYNAMIC_STACK
            if (task->stack_size < MAX_STACK_SIZE) {
                resize_stack(task, task->stack_size + STACK_STEP_SIZE);
            } else {
                task->state = TASK_SUSPENDED;
            }
#else
            task->state = TASK_SUSPENDED;
#endif
        }
    }
#ifdef PRINT
    printf("\nCalculating stack size took: %u us\n", time_us_32() - start_time);
#endif
#ifdef PROFILE_SCHEDULER
    profile.time_stack_metrics += (time_us_32() - start_time) / 1000.0f;
#endif
    scheduler_spin_unlock(saved_irq);
}

void get_next_task() {
    const uint32_t saved_irq = scheduler_spin_lock();
#if defined(PROFILE_SCHEDULER) || defined(PRINT)
    uint32_t start_time = time_us_32();
#endif

    scheduler_t *scheduler = get_scheduler();

    uint8_t highest_priority = 0;

    if (scheduler->current_task->state == TASK_RUNNING) {
        scheduler->current_task->state = TASK_READY;    // tell scheduler that the old task is not running anymore
                                                        // when we move to dual-core, this will be useful
    }

    uint32_t potential_index = scheduler->current_task_index;
    absolute_time_t now = get_absolute_time();

    for (uint32_t t = 0; t < MAX_TASKS; t++) {
        potential_index++;

        if (potential_index >= MAX_TASKS) {
            potential_index = 0;
        }

        task_t *potential_task = &tasks[potential_index];

        // if this task is free, skip
        if (potential_task->state == TASK_FREE) {
            continue;
        }

        // see if the required time has passed for this task, if it was waiting
        if (potential_task->state == TASK_WAIT_US) {
            if (absolute_time_diff_us(now, potential_task->resume_us) <= 0) {
                potential_task->state = TASK_READY;
            }
        }

        // if this task has the highest priority found so far, select it
        if ((potential_task->state == TASK_READY) && (potential_task->priority > highest_priority)) {
            if (find_and_resolve_stack_overflow(potential_task)) {
                highest_priority = potential_task->priority;
                scheduler->current_task_index = potential_index;
                scheduler->current_task = potential_task;
            }
        }

        // if this task was yielding, reset it to ready
        // this is done after the task is chosen, as if a high priority task yields
        // we want to run a lower priority task before running the high priority one again
        if (potential_task->state == TASK_YIELDING) {
            potential_task->state = TASK_READY;
        }
    }
#ifdef PRINT
    printf("Finding next task took: %u us\n", time_us_32() - start_time);
    printf("Loading task id: %u\n", scheduler->current_task->id);
#endif
    scheduler->current_task->state = TASK_RUNNING; // tell scheduler that the new task is running
#ifdef PROFILE_SCHEDULER
    if (CORE_NUM == 0) {
        profile.time_scheduling += (time_us_32() - start_time) / 1000.0f;
    }
#endif
    scheduler_spin_unlock(saved_irq);
}

void isr_hardfault(void) {
#ifdef STATUS_LED
    gpio_init(STATUS_LED_PIN);
    gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
    gpio_put(STATUS_LED_PIN, false);
#endif
    asm("bkpt");
}

void raise_pendsv() {
    *(volatile uint32_t *)(0xe0000000|M0PLUS_ICSR_OFFSET) = (1L<<28);
}

void isr_systick(void) {
#ifdef PROFILE_SCHEDULER
    uint32_t start_time = time_us_32();
#endif
    scheduler_t *scheduler = get_scheduler();
    scheduler->ticks_executing++;
    task_t *task = get_current_task();

    if (task->id < CORE_COUNT) {
        scheduler->ticks_idling++;
    }

    task->ticks_executing++;

    if (CORE_NUM == 0) {
        if ((scheduler->ticks_executing % STACK_MONITOR_FREQ) == 0) {
            calculate_stack_usage();
        }
        if ((scheduler->ticks_executing % 100) == 0) {
            calculate_cpu_usage();
        }
    }

#ifdef PROFILE_SCHEDULER
    profile.time_total += (time_us_32() - start_time) / 1000.0f;
#endif

#ifdef PROFILE_SCHEDULER
    if (((scheduler->ticks_executing % 500) == 0) && (CORE_NUM == 0)) {
        printf("========= Profile Report =========\n");
        printf("%f ms total scheduling\n", profile.time_total);
        printf("%f ms picking next task\n", profile.time_scheduling);
        printf("%f ms measuring stacks\n", profile.time_stack_metrics);
        printf("%f ms resizing stacks\n", profile.time_stack_resizing);
        printf("%f ms measuring CPU usage\n", profile.time_cpu_metrics);
        printf("==================================\n\n");

        profile.time_total = 0.0;
        profile.time_scheduling = 0.0;
        profile.time_stack_metrics = 0.0;
        profile.time_stack_resizing = 0.0;
        profile.time_cpu_metrics = 0.0;
        profile.time_spinlock = 0.0;
    }
#endif

    // raise PendSV interrupt (handler in assembly!)
    raise_pendsv();
}

int32_t start_systick() {
    uint32_t clock_hz = clock_get_hz(clk_sys);
    uint32_t ticks = clock_hz / 1000 * LOOP_TIME;     // loop time in ms

    // configure SysTick
    systick_hw->rvr = ticks - 1;                        // set ticks until fire
    systick_hw->cvr = 0;                                // clear current value
    systick_hw->csr = M0PLUS_SYST_CSR_CLKSOURCE_BITS |  // use system clock
                      M0PLUS_SYST_CSR_TICKINT_BITS |    // enable interrupt
                      M0PLUS_SYST_CSR_ENABLE_BITS;      // enable SysTick

    return 0;
}

void remove_task(task_t *task) {
    const uint32_t saved_irq = scheduler_spin_lock();

    task->state = TASK_DEAD;
    scheduler_spin_unlock(saved_irq);
    raise_pendsv();
}

void task_return() {
    remove_task(get_current_task());
}

int32_t add_task(void (*task_function)(uint32_t), const uint32_t id, const uint8_t priority) {
    const uint32_t saved_irq = scheduler_spin_lock();

    if (num_tasks >= MAX_TASKS) {
        PRINT_WARNING("No more open tasks.\n");
        scheduler_spin_unlock(saved_irq);
        return -1; // no more slots empty
    }

    for (int i = 0; i < num_tasks; i++) {
        if (tasks[i].id == id && tasks[i].state != TASK_FREE) {
            PRINT_WARNING("Task id already taken.\n");
            scheduler_spin_unlock(saved_irq);
            return -2; // id taken
        }
    }

    task_t *task = NULL;

    // find available task slot
    for (int t = 0; t < MAX_TASKS; t++) {
        if (tasks[t].state == TASK_FREE) {
            task = &tasks[t];
            break;
        }
    }

    if (task == NULL) {
        scheduler_spin_unlock(saved_irq);
        return -1; // mo more room for tasks
    }

    task->stack_usage = 0;
    task->cpu_usage = 0;
    task->ticks_executing = 0;
#ifdef OPTIMIZE_STACK_MONITORING
    task->stack_recalculate_cooldown = 0;
#endif

    task->id = id;
    task->priority = priority;

#ifdef DYNAMIC_STACK
    const uint32_t stack_size = STARTING_STACK_SIZE;
#else
    const uint32_t stack_size = STACK_SIZE;
#endif
    task->stack = (uint32_t*)malloc(stack_size * sizeof(uint32_t)); // dynamically get stack from heap
    task->stack_size = stack_size; // in 32bit words
    uint32_t *stack_top = task->stack; // lowest value in stack
    task->stack_base = stack_top + task->stack_size - 1; // highest value in stack (where the sp starts)

    // fill stack with known values for stack monitoring
    for (uint32_t i = 0; i < stack_size; i++) {
        task->stack[i] = STACK_FILLER;
    }

    // save current stack pointer position
    task->stack_pointer = task->stack_base;

    // set up initial stack frame for context switching
    *(task->stack_pointer--) = (uint32_t)0x01000000;  // PSR (Thumb bit set)
    *(task->stack_pointer--) = (uint32_t)task_function;  // PC (where to start)
    *(task->stack_pointer--) = (uint32_t)task_return;  // LR (return address)
    *(task->stack_pointer--) = 12;          // R12
    *(task->stack_pointer--) = 3;           // R3
    *(task->stack_pointer--) = 2;           // R2
    *(task->stack_pointer--) = 1;           // R1
    *(task->stack_pointer--) = task->id;    // R0 (pass id to task)

    // I'm leaving these here as a testimony of frustration
    // These are pushed by C onto the stack upon entering the interrupt handler
    // They would be an issue, except for the fact I'm now using the dual-stack-pointer
    // functionality of the Cortex-M0, so they are pushed onto the MSP stack, not the process stack.
    // I hope...
    // *(task.stack_pointer--) = (uint32_t)task_function; // LR (C decided to push these! :( )
    // *(task.stack_pointer--) = 0;           // R4

    *(task->stack_pointer--) = 7;           // R7
    *(task->stack_pointer--) = 6;           // R6
    *(task->stack_pointer--) = 5;           // R5
    *(task->stack_pointer--) = 4;           // R4
    *(task->stack_pointer--) = 11;          // R11
    *(task->stack_pointer--) = 10;          // R10
    *(task->stack_pointer--) = 9;           // R9
    *(task->stack_pointer) = 8;             // R8

    task->state = TASK_READY;

    num_tasks++;
    scheduler_spin_unlock(saved_irq);
    return 0;
}

void idle_task(uint32_t pid) {
    while (true) {
        __wfi();
    }
}

void start_scheduler_this_core() {

    add_task(idle_task, 0 + CORE_NUM, 0);

    if (num_tasks == 0) {
        PRINT_WARNING("No tasks to run\n");
        return; // no tasks
    }

    PRINT_DEBUG("Starting Scheduler\n");

    get_next_task();

    PRINT_DEBUG("Adding Timer...\n");

    start_systick();

    PRINT_DEBUG("Timer Started!\n");
}

int32_t start_kernel() {
#ifdef STATUS_LED
    gpio_init(STATUS_LED_PIN);
    gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
    gpio_put(STATUS_LED_PIN, true);
#endif
    init_spin_locks();
#ifdef PROFILE_SCHEDULER
    profile.time_total = 0.0;
    profile.time_scheduling = 0.0;
    profile.time_stack_metrics = 0.0;
    profile.time_stack_resizing = 0.0;
    profile.time_cpu_metrics = 0.0;
    profile.time_spinlock = 0.0;
#endif
#if CORE_COUNT > 1
    multicore_reset_core1();
    multicore_launch_core1(start_scheduler_this_core);
#endif
    start_scheduler_this_core();
    set_spsel(2);
    return 0;
}

/* Task Functions */

void task_sleep_ms(uint32_t ms) {
    task_sleep_us(ms * 1000);
}

void task_sleep_us(uint64_t us) {
    const uint32_t saved_irq = scheduler_spin_lock();
    task_t *current_task = get_current_task();
    current_task->state = TASK_WAIT_US;
    current_task->resume_us = make_timeout_time_us(us);
    scheduler_spin_unlock(saved_irq);
    raise_pendsv();
}

void task_yield() {
    const uint32_t saved_irq = scheduler_spin_lock();
    task_t *current_task = get_current_task();
    current_task->state = TASK_YIELDING;
    scheduler_spin_unlock(saved_irq);
    raise_pendsv();
}

