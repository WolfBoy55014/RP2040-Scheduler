#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <pico/time.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include "scheduler.h"

void task_one() {
    gpio_init(8);
    gpio_set_dir(8, GPIO_OUT);

    while (true) {
        gpio_put(8, true);
        busy_wait_ms(200);
        gpio_put(8, false);
        busy_wait_ms(200);
    }
}

void task_two() {
    gpio_init(9);
    gpio_set_dir(9, GPIO_OUT);

    while (true) {
        gpio_put(9, true);
        busy_wait_ms(340);
        gpio_put(9, false);
        busy_wait_ms(340);
    }
}

int main() {
    stdio_init_all();

    addTask(task_one, 1);
    addTask(task_two, 2);

    startScheduler();

    while (true) {
        tight_loop_contents();
    } // don't return
}
 