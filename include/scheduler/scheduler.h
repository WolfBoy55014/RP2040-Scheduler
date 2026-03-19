#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

/* Task Signals */
#define TASK_SIGTERM (1 << 1) // graceful shutdown
#define TASK_SIGKILL (1 << 2) // force shutdown
#define TASK_SIGSTOP (1 << 3) // pause execution
#define TASK_SIGCONT (1 << 4) // continue execution
#define TASK_SIGEXCP (1 << 5)
#define TASK_SIGWTDG (1 << 6)
#define TASK_SIGUSR1 (1 << 7)
#define TASK_SIGUSR2 (1 << 8)

/* Basic Scheduler Functions */
int32_t kernel_start();
int32_t task_add_args(void (*task_function)(uint32_t, uint32_t*, char*), uint32_t id, char* args, uint8_t priority);
int32_t task_add(void (*task_function)(uint32_t, uint32_t*, char*), uint32_t id, uint8_t priority);

// task management
/**
 * Make the current task sleep for ms milliseconds
 * @param ms the amount of milliseconds to sleep
 */
void task_sleep_ms(uint32_t ms);

/**
 * Make the current task sleep for us microseconds
 * @param us the amount of milliseconds to sleep
 */
void task_sleep_us(uint64_t us);

/**
 * Make the current task yield for at least one scheduler loop
 * Will make room for lower-priority tasks to run be for this one
 */
void task_yield();

void task_end(int32_t code);

/**
 * Check if a task with a certain id exists
 * @param pid The id of the task to look for
 * @return Whether the task exists
 */
bool task_exists(uint32_t pid);

/**
 * Set the signals flags of a task
 * This can not clear signals, but only raise them
 * @param pid The id of the task for the signal to be sent to
 * @param signals a 32-bit integer containing the signals
 */
void task_signal(uint32_t pid, uint32_t signals);

#endif //SCHEDULER_H