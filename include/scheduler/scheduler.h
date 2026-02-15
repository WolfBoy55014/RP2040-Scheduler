#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

/* Basic Scheduler Functions */
int32_t kernel_start();
int32_t task_add(void (*task_function)(uint32_t), uint32_t id, uint8_t priority);

// task management
void task_sleep_ms(uint32_t ms);
void task_sleep_us(uint64_t us);
void task_yield();
void task_end(int32_t code);
bool task_exists(uint32_t pid);

#endif //SCHEDULER_H