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
#define CORE_COUNT 1            // number of cores the kernel will use
#endif

#ifndef MAX_TASKS
#define MAX_TASKS 16            // max number of tasks the kernel will accommodate
                                // (include an extra for each core for idle tasks)
#endif

#ifndef LOOP_TIME
#define LOOP_TIME 1             // cycle time in ms, may be fractional
                                // (optimal value was found to be between 1-3 ms)
#endif

#ifndef USE_GOVERNOR
#define USE_GOVERNOR 1          // run the cpu frequency governor
#endif

#ifndef GOVERNOR_LIMIT_SAFE
#define GOVERNOR_LIMIT_SAFE 1   // limit the governor in how much it will overclock
#endif

#ifndef CPU_USAGE_PERIOD
#define CPU_USAGE_PERIOD 127    // calculate average cpu usage every this many ticks
#endif

#if USE_GOVERNOR
#ifndef GOVERNOR_PERIOD
#define GOVERNOR_PERIOD 251      // run the governor every this many ticks
#endif
#endif

#ifndef SCHEDULER_GARBAGE_COLLECT_PERIOD
#define SCHEDULER_GARBAGE_COLLECT_PERIOD 7     // run the scheduler garbage collector every this many ticks
#endif


// --- Stack Configs ---

// all stack sizes are measured in 32 bit *words*
// so a stack is 4 times larger in *bytes*

#ifndef DYNAMIC_STACK
#define DYNAMIC_STACK 1         // perform dynamic re-allocation of stacks as they grow
#endif

#if DYNAMIC_STACK
#ifndef STARTING_STACK_SIZE
#define STARTING_STACK_SIZE 128 // the amount of stack that is initially given to a task
#endif

#ifndef MIN_STACK_SIZE
#define MIN_STACK_SIZE 64       // the minimum amount of stack the scheduler will resize down to
#endif

#ifndef MAX_STACK_SIZE
#define MAX_STACK_SIZE 8192     // the maximum amount of stack the scheduler will give a task before suspending it
#endif

#ifndef STACK_STEP_SIZE
#define STACK_STEP_SIZE 32      // the amount of stack the scheduler will give and take from a task at a time
                                // (preferably fits nicely into MAX_STACK_SIZE - MIN_STACK_SIZE)
                                // (you might want to try increasing this if you expect rapid stack usage
                                // or are experiencing random crashes)
#endif

#else
#ifndef STACK_SIZE
#define STACK_SIZE 256          // stack size of each task. a task will be suspended if going over this limit
#endif
#endif

#ifndef STACK_OVERFLOW_THRESHOLD
#define STACK_OVERFLOW_THRESHOLD 32 // a task will be suspended or have its stack resized
                                    // when it has less than this many words left free in its stack
                                    // (you might want to try increasing this if you expect rapid stack usage
                                    // or are experiencing random crashes)
                                    // not used if using hardware stack protection
#endif

#ifndef STACK_MONITOR_PERIOD
#define STACK_MONITOR_PERIOD 101// perform a stack usage calculation every this many ticks
                                // can add large overhead to the scheduler which can be roughly calculated:
                                // MAX_TASKS * STACK_SIZE * 0.0079 ms (or about 2 ms per task)
                                // (this could be set very high if you think your tasks won't need more stack)
                                // (if you do experience crashes, you can try reducing this,
                                // but try STACK_OVERFLOW_THRESHOLD first)
#endif

#ifndef STACK_FILLER
#define STACK_FILLER 0x1ABE11ED // what the stack is filled with to measure stack usage
#endif

#define OPTIMIZE_STACK_MONITORING 1 // apply optimizations to stack monitoring
                                    // such as reducing the amount of calculations spent
                                    // on tasks with low stack usage

#if OPTIMIZE_STACK_MONITORING
#ifndef OPTIMIZE_STACK_MONITORING_FACTOR
#define OPTIMIZE_STACK_MONITORING_FACTOR 7  // how many recalculations to delay recalculating a tasks usage
                                            // per STACK_OVERFLOW_THRESHOLDs of free space
                                            // (if you increase STACK_OVERFLOW_THRESHOLD,
                                            // you might want to decrease this)
#endif
#endif

#ifndef ALIGN_STACK_ALLOCATIONS
#define ALIGN_STACK_ALLOCATIONS 1
#endif

#ifndef STACK_ALIGNMENT_SIZE
#define STACK_ALIGNMENT_SIZE 256
#endif

#ifndef USE_HARDWARE_STACK_GUARDS
#define USE_HARDWARE_STACK_GUARDS 1
#endif

#define USE_MPU (PICO_RP2040 && USE_HARDWARE_STACK_GUARDS)

#if PICO_RP2040 && USE_HARDWARE_STACK_GUARDS
#if !ALIGN_STACK_ALLOCATIONS | (STACK_ALIGNMENT_SIZE != 256)
#error The stacks must be aligned to 256 byte boundaries for mpu stack protection on the RP2040!
#endif
#endif

// --- Channel Configs ---

#ifndef NUM_CHANNELS
#define NUM_CHANNELS 16          // number of communication channels (must be less that the 16-bit integer limit)
#endif

#ifndef CHANNEL_SIZE
#define CHANNEL_SIZE 16         // length of each fifo in a communication channel (in bytes) (must be less that the 16-bit integer limit)
#endif

#ifndef CHANNEL_AUTO_FREE_DELAY
#define CHANNEL_AUTO_FREE_DELAY 1000 // how many milliseconds have to pass before a channel will be automatically freed
#endif

#ifndef CHANNEL_GARBAGE_COLLECT_PERIOD
#define CHANNEL_GARBAGE_COLLECT_PERIOD 103 // the channel garbage collector runs every this many ticks
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

#ifndef PRINT
#define PRINT 0              // either or not to print debug messages
#endif

#if PRINT
#define PRINT_WARNING(msg) printf(msg)
#define PRINT_DEBUG(msg) printf(msg)
#else
#define PRINT_WARNING(msg)
#define PRINT_DEBUG(msg)
#endif

#ifndef PROFILE_SCHEDULER
#define PROFILE_SCHEDULER 0
#endif

#ifndef STATUS_LED
#define STATUS_LED 1
#endif

#if STATUS_LED
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 25
#endif
#endif

#endif

#endif //KERNEL_CONFIG_H
