//
// Created by wolfboy on 8/24/25.
//

// KERNAL CONFIG FILE
// WHOLE THING CAN BE OVERRIDDEN BY DEFINING
// `CUSTOM_KERNEL_CONFIG`
// IF SO, PLEASE USE THIS AS A TEMPLATE

#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#ifndef CUSTOM_KERNEL_CONFIG

// --- Scheduler Configs ---

#ifndef CORE_COUNT
#define CORE_COUNT 2            // number of cores the kernel will use
#endif

#ifndef MAX_TASKS
#define MAX_TASKS 8             // max number of tasks the kernel will accommodate
                                // (include an extra for each core for idle tasks)
#endif

#ifndef LOOP_TIME
#define LOOP_TIME 2             // cycle time in ms, may be fractional
                                // (optimal value was found to be between 1-3 ms)
#endif

// --- Stack Configs ---

// all stack sizes are measured in 32 bit *words*
// so a stack is 4 times larger in *bytes*

#define DYNAMIC_STACK           // perform dynamic re-allocation of stacks as they grow

#ifdef DYNAMIC_STACK
#ifndef STARTING_STACK_SIZE
#define STARTING_STACK_SIZE 256 // the amount of stack that is initially given to a task
#endif

#ifndef MIN_STACK_SIZE
#define MIN_STACK_SIZE 64       // the minimum amount of stack the scheduler will resize down to
#endif

#ifndef MAX_STACK_SIZE
#define MAX_STACK_SIZE 1024     // the maximum amount of stack the scheduler will give a task before suspending it
#endif

#ifndef STACK_STEP_SIZE
#define STACK_STEP_SIZE 32      // the amount of stack the scheduler will give and take from a task at a time
                                // (preferably fits nicely into MAX_STACK_SIZE - MIN_STACK_SIZE)
#endif
#else
#define STACK_SIZE 256          // stack size of each task. a task will be suspended if going over this limit
#endif

#ifndef STACK_OVERFLOW_THRESHOLD
#define STACK_OVERFLOW_THRESHOLD 24 // a task will be suspended or have its stack resized
                                    // when it has less than this many words left free in its stack
#endif

#ifndef STACK_FILLER
#define STACK_FILLER 0x1ABE11ED // what the stack is filled with to measure stack usage
#endif

// --- Channel Configs ---

#ifndef NUM_CHANNELS
#define NUM_CHANNELS 32         // number of communication channels (must be less that the 16-bit integer limit)
#endif

#ifndef CHANNEL_SIZE
#define CHANNEL_SIZE 16         // length of each fifo in a communication channel (in bytes)
#endif

// --- Spinlock Configs ---

#ifndef SCHEDULER_SPINLOCK_ID
#define SCHEDULER_SPINLOCK_ID PICO_SPINLOCK_ID_OS1  // no need to usually touch this,
                                                    // as this spinlock is reserved for OSs
#endif

#ifndef CHANNEL_SPINLOCK_ID
#define CHANNEL_SPINLOCK_ID   PICO_SPINLOCK_ID_OS2  // no need to usually touch this,
                                                    // as this spinlock is reserved for OSs
#endif

// --- Debug ---

// #define PRINT                   // either or not to print debug messages

#ifdef PRINT
#define PRINT_WARNING(msg) printf(msg)
#define PRINT_DEBUG(msg) printf(msg)
#else
#define PRINT_WARNING(msg)
#define PRINT_DEBUG(msg)
#endif

#define STATUS_LED

#ifdef STATUS_LED
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 25
#endif
#endif

#endif

#endif //KERNEL_CONFIG_H
