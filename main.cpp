#include <hardware/gpio.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include "scheduler.hpp"

void task_one() {
    gpio_init(8);
    gpio_set_dir(8, GPIO_OUT);

    while (true) {
        gpio_put(8, true);
        sleep_ms(100);
        gpio_put(8, false);
        sleep_ms(100);
    }
}

void task_two() {
    gpio_init(9);
    gpio_set_dir(9, GPIO_OUT);

    while (true) {
        gpio_put(9, true);
        sleep_ms(200);
        gpio_put(9, false);
        sleep_ms(200);
    }
}

int main() {
    stdio_init_all();

    initScheduler();
    startScheduler();
    
    while(true); // don't return
}
 