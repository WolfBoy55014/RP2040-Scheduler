#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/structs/io_bank0.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include "scheduler.h"
#include "task.h"

void digit_to_pins(bool *pins, uint8_t digit) {
    pins[0] = 0;
    pins[1] = 0;
    pins[2] = 0;
    pins[3] = 0;
    pins[4] = 0;
    pins[5] = 0;
    pins[6] = 0;
    pins[7] = 0;

    switch (digit) {
        case 0:
            pins[0] = 1;
            pins[1] = 1;
            pins[2] = 1;
            pins[4] = 1;
            pins[5] = 1;
            pins[6] = 1;
            break;
        case 1:
            pins[2] = 1;
            pins[4] = 1;
            break;
        case 2:
            pins[0] = 1;
            pins[1] = 1;
            pins[4] = 1;
            pins[5] = 1;
            pins[7] = 1;
            break;
        case 3:
            pins[1] = 1;
            pins[2] = 1;
            pins[4] = 1;
            pins[5] = 1;
            pins[7] = 1;
            break;
        case 4:
            pins[2] = 1;
            pins[4] = 1;
            pins[6] = 1;
            pins[7] = 1;
            break;
        case 5:
            pins[1] = 1;
            pins[2] = 1;
            pins[5] = 1;
            pins[6] = 1;
            pins[7] = 1;
            break;
        case 6:
            pins[0] = 1;
            pins[1] = 1;
            pins[2] = 1;
            pins[5] = 1;
            pins[6] = 1;
            pins[7] = 1;
            break;
        case 7:
            pins[2] = 1;
            pins[4] = 1;
            pins[5] = 1;
            break;
        case 8:
            pins[0] = 1;
            pins[1] = 1;
            pins[2] = 1;
            pins[4] = 1;
            pins[5] = 1;
            pins[6] = 1;
            pins[7] = 1;
            break;
        case 9:
            pins[1] = 1;
            pins[2] = 1;
            pins[4] = 1;
            pins[5] = 1;
            pins[6] = 1;
            pins[7] = 1;
            break;
        default: ;
    }
}

void task_one(uint32_t pid) {
    bool digit_pins[8];

    for (uint32_t p = 12; p <= 19; p++) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
    }

    while (true) {
        for (uint8_t i = 0; i <= 9; i++) {
            digit_to_pins(digit_pins, i);

            for (uint32_t p = 12; p <= 19; p++) {
                gpio_put(p, digit_pins[p - 12]);
            }

            task_sleep_ms(500);
        }
    }
}

void task_two(uint32_t pid) {
    gpio_set_function(pid, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pid);

    pwm_set_wrap(slice_num, 127);
    pwm_set_gpio_level(pid, 0);
    pwm_set_enabled(slice_num, true);

    while (true) {
        for (uint16_t i = 0; i < 127; i++) {
            pwm_set_gpio_level(pid, i);
            task_sleep_ms(20);
        }

        for (uint16_t i = 127; i > 0; i--) {
            pwm_set_gpio_level(pid, i);
            task_sleep_ms(20);
        }

        task_yield();
    }
}


void recursive_function(int depth) {
    char large_array[8]; // Allocate a local array to consume more stack per call
    large_array[0] = 1;
    if (depth > 0) {
        recursive_function(depth - 1);
    }
}

void stack_overflow_task(uint32_t pid) {
    recursive_function(1000); // Call with a large depth
}

int main() {
    stdio_init_all();

    addTask(task_one, 9, 2);
    addTask(task_two, 10, 2);
    // addTask(stack_overflow_task, 7, 1);

    startScheduler();

    while (true) {
        tight_loop_contents();
    } // don't return
}
 