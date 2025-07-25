#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/gpio.h>
#include "scheduler.h"
#include <hardware/clocks.h>
#include <hardware/structs/systick.h>

extern void set_spsel(uint32_t control);
extern void context_save(task_t *task);
extern void context_load(task_t *task);

volatile task_t *current_task;
volatile task_t task_list[MAX_TASKS];
volatile uint32_t current_task_index = 0;
volatile uint32_t num_tasks = 0;
volatile uint32_t scheduler_started = false;

void getNextTask() {
    current_task_index = (current_task_index + 1) % num_tasks;
#ifdef PRINT
    printf("Loading task id: %u", current_task_index);
#endif
    current_task = &task_list[current_task_index];
}

void isr_systick(void) {
    LED_BLINK(LED_DEBUG_PIN);

    // raise PendSV interrupt
    *(volatile uint32_t *)(0xe0000000|M0PLUS_ICSR_OFFSET) = (1L<<28);
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

struct repeating_timer timer;

uint32_t startScheduler() {

    LED_INIT(LED_DEBUG_PIN);
    LED_INIT(LED_WARN_PIN);
    LED_INIT(LED_FATAL_PIN);

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

uint32_t addTask(void (*task_function)(void), uint32_t id) {

    if (num_tasks >= MAX_TASKS) {
        LED_FLAG(LED_WARN_PIN);
        PRINT_WARNING("No more open tasks.");
        return -1; // no more slots empty
    }

    for (int i = 0; i < num_tasks; i++) {
        if (task_list[i].id == id) {
            LED_FLAG(LED_WARN_PIN);
            PRINT_WARNING("Task id already taken.");
            return -2; // id taken
        }
    }

    task_t *task = &task_list[num_tasks];
    task->id = id;
    task->state = RUNNING;

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
    *(task->stack_pointer--) = 0;           // R0

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