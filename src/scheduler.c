#include <stdlib.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "hardware/sync.h"
#include "hardware/structs/systick.h"
#include "pico/multicore.h"

#include "scheduler_internal.h"
#include "scheduler.h"

#include <stdio.h>

#include "spinlock_internal.h"

/* Scheduler Variables */
volatile scheduler_t schedulers[CORE_COUNT];
volatile uint32_t num_tasks;
volatile task_t tasks[MAX_TASKS];

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

void calculate_cpu_usage(scheduler_t *scheduler) {
    const uint32_t saved_irq = scheduler_spin_lock();
    scheduler->total_cpu_usage = 0;

#ifdef PRINT
    printf("================ CPU Usage ================\n");
#endif
    for (uint32_t t = 0; t < num_tasks; t++) {
        task_t *task = &tasks[t];
        uint8_t cpu_usage = (task->ticks_executing * 100) / scheduler->total_ticks_executing;
        task->cpu_usage = cpu_usage;
        if (task->priority != 0) {
            scheduler->total_cpu_usage += cpu_usage;
        }
        task->ticks_executing = 0;

#ifdef PRINT
        printf("PID: %u, USAGE: %u%%\n", task->id, task->cpu_usage);
#endif
    }
#ifdef PRINT
    printf("Total Usage: %u%%\n", scheduler->total_cpu_usage);
    printf("===========================================\n\n");
#endif

    scheduler->total_ticks_executing = 0;
    scheduler_spin_unlock(saved_irq);
}

void calculate_stack_usage(scheduler_t *scheduler) {
    const uint32_t saved_irq = scheduler_spin_lock();
#ifdef PRINT
    printf("=============== Stack Usage ===============\n");
#endif
    for (uint32_t t = 0; t < num_tasks; t++) {
        task_t *task = &tasks[t];
        uint32_t bytes_used = 0;

        for (uint32_t i = 0; i < STACK_SIZE; i++) {
            if (task->stack[i] != STACK_FILLER) {
                bytes_used++;
            }
        }

        task->stack_usage = (bytes_used * 100) / STACK_SIZE;

        // stack overflow ;)
        if (bytes_used >= STACK_SIZE && task->state != TASK_RUNNING) {
            LED_FLAG(LED_WARN_PIN);
            task->state = TASK_SUSPENDED;
        }
#ifdef PRINT
        printf("PID: %u, USAGE: %u%%\n", task->id, task->stack_usage);
#endif
    }
#ifdef PRINT
    printf("===========================================\n\n");
#endif
    scheduler_spin_unlock(saved_irq);
}

void get_next_task() {
    const uint32_t saved_irq = scheduler_spin_lock();
#ifdef PRINT
    uint64_t start_time = time_us_64();
#endif

    scheduler_t *scheduler = get_scheduler();

    uint8_t highest_priority = 0;

    if (scheduler->current_task->state == TASK_RUNNING) {
        scheduler->current_task->state = TASK_READY;    // tell scheduler that the old task is not running anymore
                                                        // when we move to dual core, this will be useful
    }

    for (uint32_t t = 1; t <= num_tasks; t++) {
        uint32_t potential_index = (t + scheduler->current_task_index) % num_tasks;

        task_t *potential_task = &tasks[potential_index];

        // if task is dead, remove it
        if (potential_task->state == TASK_DEAD) {
            free(potential_task->stack);
            potential_task->state = TASK_FREE;
        }

        // see if the required time has passed for this task, if it was waiting
        if (potential_task->state == TASK_WAIT_US) {
            if (absolute_time_diff_us(get_absolute_time(), potential_task->resume_us) <= 0) {
                potential_task->state = TASK_READY;
            }
        }

        // get the highest priority of the ready tasks
        if (potential_task->state == TASK_READY) {
            if (potential_task->priority > highest_priority) {
                highest_priority = potential_task->priority;
            }
        }

        // if this task was yielding, reset it to running
        // this is done after the task is chosen, as if a high priority task yields
        // we want to run a lower priority task before running the high priority one again
        if (potential_task->state == TASK_YIELDING) {
            potential_task->state = TASK_READY;
        }
    }

    // get the next task with the highest priority
    for (uint32_t t = 1; t <= num_tasks; t++) {
        uint32_t potential_index = (t + scheduler->current_task_index) % num_tasks;

        task_t *potential_task = &tasks[potential_index];

        if (potential_task->state == TASK_READY && potential_task->priority == highest_priority) {
            scheduler->current_task_index = potential_index;
            scheduler->current_task = potential_task;
            break;
        }
#ifdef PRINT
        // printf("Finding next task took: %llus\n", time_us_64() - start_time);
#endif
    }

#ifdef PRINT
    // printf("Loading task id: %u\n", current_task->id);
#endif
    scheduler->current_task->state = TASK_RUNNING; // tell scheduler that the new task is running
    scheduler_spin_unlock(saved_irq);
}

void isr_hardfault(void) {
    LED_FLAG(LED_FATAL_PIN);
}

void raise_pendsv() {
    *(volatile uint32_t *)(0xe0000000|M0PLUS_ICSR_OFFSET) = (1L<<28);
}

void isr_systick(void) {
    LED_BLINK(LED_DEBUG_PIN);

    scheduler_t *scheduler = get_scheduler();
    scheduler->total_ticks_executing++;
    scheduler->current_task->ticks_executing++;

    if ((scheduler->total_ticks_executing % 100) == 0 && CORE_NUM == 0) {
        calculate_cpu_usage(scheduler);
        calculate_stack_usage(scheduler);
    }

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

int32_t add_task(void (*task_function)(uint32_t), uint32_t id, uint8_t priority) {
    const uint32_t saved_irq = scheduler_spin_lock();

    if (num_tasks >= MAX_TASKS) {
        LED_FLAG(LED_WARN_PIN);
        PRINT_WARNING("No more open tasks.\n");
        scheduler_spin_unlock(saved_irq);
        return -1; // no more slots empty
    }

    for (int i = 0; i < num_tasks; i++) {
        if (tasks[i].id == id && tasks[i].state != TASK_FREE) {
            LED_FLAG(LED_WARN_PIN);
            PRINT_WARNING("Task id already taken.\n");
            scheduler_spin_unlock(saved_irq);
            return -2; // id taken
        }
    }

    task_t *task = NULL;

    // find available task slot
    for (int t = 0; t < MAX_TASKS; ++t) {
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

    task->id = id;
    task->priority = priority;

    task->stack = (uint32_t*)malloc(STACK_SIZE * sizeof(uint32_t)); // dynamically get stack from heap
    task->stack_size = STACK_SIZE; // in 32bit words
    uint32_t *stack_top = task->stack; // lowest value in stack
    task->stack_base = stack_top + task->stack_size - 1; // highest value in stack (where the sp starts)

    // fill stack with known values for stack monitoring
    for (uint32_t i = 0; i < STACK_SIZE; i++) {
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
    LED_INIT(LED_DEBUG_PIN);
    LED_INIT(LED_WARN_PIN);
    LED_INIT(LED_FATAL_PIN);

    add_task(idle_task, 0 + CORE_NUM, 0);

    if (num_tasks == 0) {
        LED_FLAG(LED_WARN_PIN);
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
    init_spin_locks();
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

