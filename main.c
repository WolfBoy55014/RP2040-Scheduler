#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/structs/io_bank0.h>
#include <hardware/timer.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include "scheduler.h"
#include "task.h"

bool digit_pins[8];

void digit_to_pins(uint8_t digit) {
    digit_pins[0] = 0;
    digit_pins[1] = 0;
    digit_pins[2] = 0;
    digit_pins[3] = 0;
    digit_pins[4] = 0;
    digit_pins[5] = 0;
    digit_pins[6] = 0;
    digit_pins[7] = 0;

    switch (digit) {
        case 0:
            digit_pins[0] = 1;
            digit_pins[1] = 1;
            digit_pins[2] = 1;
            digit_pins[4] = 1;
            digit_pins[5] = 1;
            digit_pins[6] = 1;
            break;
        case 1:
            digit_pins[2] = 1;
            digit_pins[4] = 1;
            break;
        case 2:
            digit_pins[0] = 1;
            digit_pins[1] = 1;
            digit_pins[4] = 1;
            digit_pins[5] = 1;
            digit_pins[7] = 1;
            break;
        case 3:
            digit_pins[1] = 1;
            digit_pins[2] = 1;
            digit_pins[4] = 1;
            digit_pins[5] = 1;
            digit_pins[7] = 1;
            break;
        case 4:
            digit_pins[2] = 1;
            digit_pins[4] = 1;
            digit_pins[6] = 1;
            digit_pins[7] = 1;
            break;
        case 5:
            digit_pins[1] = 1;
            digit_pins[2] = 1;
            digit_pins[5] = 1;
            digit_pins[6] = 1;
            digit_pins[7] = 1;
            break;
        case 6:
            digit_pins[0] = 1;
            digit_pins[1] = 1;
            digit_pins[2] = 1;
            digit_pins[5] = 1;
            digit_pins[6] = 1;
            digit_pins[7] = 1;
            break;
        case 7:
            digit_pins[2] = 1;
            digit_pins[4] = 1;
            digit_pins[5] = 1;
            break;
        case 8:
            digit_pins[0] = 1;
            digit_pins[1] = 1;
            digit_pins[2] = 1;
            digit_pins[4] = 1;
            digit_pins[5] = 1;
            digit_pins[6] = 1;
            digit_pins[7] = 1;
            break;
        case 9:
            digit_pins[1] = 1;
            digit_pins[2] = 1;
            digit_pins[4] = 1;
            digit_pins[5] = 1;
            digit_pins[6] = 1;
            digit_pins[7] = 1;
            break;
        default: ;
    }
}

void task_one(uint32_t pid) {
    for (uint32_t p = 12; p <= 19; p++) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
    }

    while (true) {
        for (uint8_t i = 0; i <= 9; i++) {
            digit_to_pins(i);

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
        }

        for (uint16_t i = 127; i > 0; i--) {
            pwm_set_gpio_level(pid, i);
        }

        task_yield();
    }
}

int main() {
    stdio_init_all();

    addTask(task_one, 9, 2);
    addTask(task_two, 10, 3);

    startScheduler();

    while (true) {
        tight_loop_contents();
    } // don't return
}
 