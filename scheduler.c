#include <pico.h>
#include <pico/types.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <hardware/structs/systick.h>

#include "scheduler.h"

#include <math.h>

#include "task.h"

/* Public Assembly Functions */
extern void _cpsid();
extern void _cpsie();
extern void set_spsel(uint32_t control);

/* Scheduler Variables */
volatile uint32_t num_tasks = 0;
volatile task_t *current_task;
volatile uint32_t current_task_index = 0;
volatile task_t task_list[MAX_TASKS];
volatile uint32_t scheduler_started = false;

/* CPU Usage Variables */
uint64_t total_ticks_executing = 0;
uint8_t total_cpu_usage = 0;

void calculateCPUUsage() {
    total_cpu_usage = 0;

    printf("================ CPU Usage ================\n");

    for (uint32_t t = 0; t < num_tasks; t++) {
        task_t *task = &task_list[t];
        uint8_t cpu_usage = (task->ticks_executing * 100) / total_ticks_executing;
        task->cpu_usage = cpu_usage;
        if (task->id != 0) {
            total_cpu_usage += cpu_usage;
        }
        task->ticks_executing = 0;

        printf("PID: %u, USAGE: %u%%\n", task->id, task->cpu_usage);
    }

    printf("Total Usage: %u%%\n", total_cpu_usage);
    printf("===========================================\n\n");

    total_ticks_executing = 0;
}

void getNextTask() {

    uint8_t highest_priority = 0;

    if (current_task->state == RUNNING) {
        current_task->state = READY; // tell scheduler that the old task is not running anymore
                                     // when we move to dual core, this will be useful
    }

    for (uint32_t t = 1; t <= num_tasks; t++) {
        uint32_t potential_index = (t + current_task_index) % num_tasks;

        task_t *potential_task = &task_list[potential_index];

        // see if the required time has passed for this task, if it was waiting
        if (potential_task->state == WAIT_US) {
            if (absolute_time_diff_us(get_absolute_time(), potential_task->resume_us) <= 0) {
                potential_task->state = READY;
            }
        }

        // get the highest priority of the ready tasks
        if (potential_task->state == READY) {
            if (potential_task->priority > highest_priority) {
                highest_priority = potential_task->priority;
            }
        }

        // if this task was yielding, reset it to running
        // this is done after the task is chosen, as if a high priority task yields
        // we want to run a lower priority task before running the high priority one again
        if (potential_task->state == YIELDING) {
            potential_task->state = READY;
        }
    }

    // get the next task with the highest priority
    for (uint32_t t = 1; t <= num_tasks; t++) {
        uint32_t potential_index = (t + current_task_index) % num_tasks;

        task_t *potential_task = &task_list[potential_index];

        if (potential_task->state == READY && potential_task->priority == highest_priority) {
            current_task_index = potential_index;
            current_task = potential_task;
            break;
        }
    }

#ifdef PRINT
    printf("Loading task id: %u\n", current_task->id);
#endif
    current_task->state = RUNNING; // tell scheduler that the new task is running
}

inline void raisePendSV() {
    *(volatile uint32_t *)(0xe0000000|M0PLUS_ICSR_OFFSET) = (1L<<28);
}

void isr_systick(void) {
    LED_BLINK(LED_DEBUG_PIN);

    total_ticks_executing++;
    current_task->ticks_executing++;

    if ((total_ticks_executing % 5000) == 0) {
        calculateCPUUsage();
    }

    // raise PendSV interrupt (handler in assembly!)
    raisePendSV();
}

uint32_t startSysTick() {
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

uint32_t addTask(void (*task_function)(uint32_t), uint32_t id, uint8_t priority) {

    if (num_tasks >= MAX_TASKS) {
        LED_FLAG(LED_WARN_PIN);
        PRINT_WARNING("No more open tasks.\n");
        return -1; // no more slots empty
    }

    for (int i = 0; i < num_tasks; i++) {
        if (task_list[i].id == id) {
            LED_FLAG(LED_WARN_PIN);
            PRINT_WARNING("Task id already taken.\n");
            return -2; // id taken
        }
    }

    task_t *task = &task_list[num_tasks];
    task->id = id;
    task->priority = priority;
    task->state = READY;

    task->stack_size = STACK_SIZE; // in 32bit words
    uint32_t *stack_top = task->stack; // lowest value in stack
    task->stack_base = stack_top + task->stack_size - 1; // highest value in stack (where the sp starts)

    // Save current stack pointer position
    task->stack_pointer = task->stack_base;
    
    // Set up initial stack frame for context switching
    *(task->stack_pointer--) = (uint32_t)0x01000000;  // PSR (Thumb bit set)
    *(task->stack_pointer--) = (uint32_t)task_function;  // PC (where to start)
    *(task->stack_pointer--) = (uint32_t)task_function;  // LR (return address)
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

    num_tasks++;
    return 0;
}

void idleTask(uint32_t pid) {
    while (true) {
        tight_loop_contents();
    }
}

uint32_t startScheduler() {

    LED_INIT(LED_DEBUG_PIN);
    LED_INIT(LED_WARN_PIN);
    LED_INIT(LED_FATAL_PIN);

    addTask(idleTask, 0, 0);

    if (num_tasks == 0) {
        LED_FLAG(LED_WARN_PIN);
        PRINT_WARNING("No tasks to run\n");
        return -1; // no tasks
    }

    PRINT_DEBUG("Starting Scheduler\n");

    current_task_index = 0;
    current_task = &task_list[current_task_index];

    PRINT_DEBUG("Adding Timer...\n");

    startSysTick();

    PRINT_DEBUG("Timer Started!\n");

    set_spsel(2);

    return 0;
}

/* Task Functions */

void task_sleep_ms(uint32_t ms) {
    task_sleep_us(ms * 1000);
}

void task_sleep_us(uint64_t us) {
    _cpsid();
    current_task->state = WAIT_US;
    current_task->resume_us = make_timeout_time_us(us);
    _cpsie();
    raisePendSV();
}

void task_yield() {
    _cpsid();
    current_task->state = YIELDING;
    _cpsie();
    raisePendSV();
}

