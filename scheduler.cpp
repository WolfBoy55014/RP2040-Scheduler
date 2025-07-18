#include "scheduler.hpp"
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>

bool schedule(__unused struct repeating_timer *t) {
    printf("Repeat at %lld\n", time_us_64());
    return true;
}

uint32_t initScheduler() {
    LED_INIT;
    return 0;
}

uint32_t startScheduler() {

    // create the repeating interupt
    struct repeating_timer timer;
    if (!add_repeating_timer_ms(LOOP_TIME, schedule, NULL, &timer)) {
        LED_WARN;
        PRINT_WARNING("Repeating timer failed to start");
        return -1;
    }

    return 0;
}

uint32_t addTask(void (*task_function)(void), uint32_t id) {

    if (num_tasks >= MAX_TASKS) {
        LED_WARN;
        PRINT_WARNING("No more open tasks.");
        return -1; // no more slots empty
    }

    for (int i = 0; i < num_tasks; i++) {
        if (task_list[i]->id == id) {
            LED_WARN;
            PRINT_WARNING("Task id already taken.");
            return -2; // id taken
        }
    }

    task_t task;
    task.id = id;
    task.state = RUNNING;

    task.stack_size = STACK_SIZE; // in 32bit words
    uint32_t *stack_top = task.stack; // lowest value in stack
    task.stack_base = stack_top + task.stack_size - 1; // highest value in stack (where the sp starts)
    
    // Set up initial stack frame for context switching
    task.stack[task.stack_size - 1] = 0x61000000;  // PSR (Thumb bit set)
    task.stack[task.stack_size - 2] = (uint32_t)task_function;  // PC (where to start)
    task.stack[task.stack_size - 3] = (uint32_t)task_function;  // LR (return address)
    task.stack[task.stack_size - 4] = 0;           // R12
    task.stack[task.stack_size - 5] = 0;           // R3
    task.stack[task.stack_size - 6] = 0;           // R2  
    task.stack[task.stack_size - 7] = 0;           // R1
    task.stack[task.stack_size - 8] = 0;           // R0
    task.stack[task.stack_size - 9] = 0;           // R0
    task.stack[task.stack_size - 10] = 0;           // R7
    task.stack[task.stack_size - 11] = 0;           // R6
    task.stack[task.stack_size - 12] = 0;           // R5
    task.stack[task.stack_size - 13] = 0;           // R4
    task.stack[task.stack_size - 14] = 0;           // R11
    task.stack[task.stack_size - 15] = 0;           // R10
    task.stack[task.stack_size - 16] = 0;           // R9
    task.stack[task.stack_size - 17] = 0;           // R8
    
    // Save current stack pointer position
    task.stack_pointer = &task.stack[task.stack_size - 17];
    return 0;
}