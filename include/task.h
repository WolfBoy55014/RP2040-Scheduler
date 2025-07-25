//
// Created by wolfboy on 7/25/25.
//

#ifndef TASK_H
#define TASK_H

#include <pico/stdlib.h>

void task_sleep_ms(uint32_t ms);
void task_sleep_us(uint64_t us);

// Yield to scheduler
void task_yield();

#endif //TASK_H
