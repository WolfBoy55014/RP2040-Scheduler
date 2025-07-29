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
            pins[7] = 1;
            pins[6] = 1;
            break;
        case 1:
            pins[2] = 1;
            pins[4] = 1;
            break;
        case 2:
            pins[0] = 1;
            pins[2] = 1;
            pins[3] = 1;
            pins[7] = 1;
            pins[6] = 1;
            break;
        case 3:
            pins[0] = 1;
            pins[2] = 1;
            pins[3] = 1;
            pins[4] = 1;
            pins[6] = 1;
            break;
        case 4:
            pins[1] = 1;
            pins[2] = 1;
            pins[3] = 1;
            pins[4] = 1;
            break;
        case 5:
            pins[0] = 1;
            pins[1] = 1;
            pins[3] = 1;
            pins[4] = 1;
            pins[6] = 1;
            break;
        case 6:
            pins[0] = 1;
            pins[1] = 1;
            pins[3] = 1;
            pins[4] = 1;
            pins[6] = 1;
            pins[7] = 1;
            break;
        case 7:
            pins[0] = 1;
            pins[2] = 1;
            pins[4] = 1;
            break;
        case 8:
            pins[0] = 1;
            pins[1] = 1;
            pins[2] = 1;
            pins[4] = 1;
            pins[3] = 1;
            pins[6] = 1;
            pins[7] = 1;
            break;
        case 9:
            pins[0] = 1;
            pins[1] = 1;
            pins[2] = 1;
            pins[4] = 1;
            pins[3] = 1;
            pins[6] = 1;
            break;
        default: ;
    }
}

void display_digit(uint32_t *pins, uint8_t number) {
    bool values[8];
    digit_to_pins(values, number);

    for (uint32_t p = 0; p < 8; p++) {
        gpio_put(pins[p], values[p]);
    }
}

void task_one(uint32_t pid) {
    uint32_t segment_pins[8] = {18, 17, 16, 15, 14, 13, 12, 11};
    uint32_t digit_pins[4] = {21, 19, 20, 10};

    for (uint8_t i = 0; i < 4; i++) {
        gpio_init(digit_pins[i]);
        gpio_set_dir(digit_pins[i], GPIO_OUT);
        gpio_set_pulls(digit_pins[i], false, true);
        gpio_put(digit_pins[i], true);
    }

    for (uint32_t p = 0; p < 8; p++) {
        gpio_init(segment_pins[p]);
        gpio_set_dir(segment_pins[p], GPIO_OUT);
    }

    while (true) {
        for (uint8_t number = 0; number <= 9; number++) {
            for (uint32_t i = 0; i < 1000; i++) {
                for (uint8_t digit = 0; digit < 4; digit++) {
                    gpio_put(digit_pins[0], true);
                    gpio_put(digit_pins[1], true);
                    gpio_put(digit_pins[2], true);
                    gpio_put(digit_pins[3], true);
                    gpio_put(digit_pins[digit], false);
                    display_digit(segment_pins, number);
                    task_sleep_us(100);
                }
            }
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
        for (uint16_t i = 0; i < 31; i++) {
            pwm_set_gpio_level(pid, i);
            task_sleep_ms(20);
        }

        for (uint16_t i = 31; i > 0; i--) {
            pwm_set_gpio_level(pid, i);
            task_sleep_ms(20);
        }

        task_yield();
    }
}

void recursive_function(int depth) {
    uint32_t bread = 9;
    bread++;
    if (depth > 0) {
        task_sleep_ms(100);
        recursive_function(depth - 1);
    }
}

void stack_overflow_task(uint32_t pid) {
    recursive_function(1000); // Call with a large depth
    while (true);
}

int main() {
    stdio_init_all();

    addTask(task_one, 10, 3);
    addTask(task_two, 9, 2);
    addTask(stack_overflow_task, 8, 2);

    startScheduler();

    while (true) {
        tight_loop_contents();
    } // don't return
}
 