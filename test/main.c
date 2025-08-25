#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <stdio.h>
#include <pico/stdlib.h>

#include "kernel_config.h"
#include "channel.h"
#include "md5.h"
#include "scheduler.h"

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

void display_number(uint32_t segment_pins[8], uint32_t digit_pins[4], uint16_t number) {
    for (uint8_t digit = 0; digit < 4; digit++) {
        gpio_put(digit_pins[0], true);
        gpio_put(digit_pins[1], true);
        gpio_put(digit_pins[2], true);
        gpio_put(digit_pins[3], true);
        display_digit(segment_pins, number % 10);
        gpio_put(digit_pins[digit], false);
        number /= 10;
        task_sleep_us(100);
    }
}

void task_display(uint32_t pid) {
    uint32_t segment_pins[8] = {18, 17, 16, 15, 14, 13, 12, 11};
    uint32_t digit_pins[4] = {10, 20, 19, 21};

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

    uint16_t number = 0;

    while (true) {
        uint16_t connected_channels[NUM_CHANNELS];
        if (get_connected_channels(connected_channels, sizeof(connected_channels) / sizeof(uint16_t)) > 0) {
            if (channel_ready_to_read(connected_channels[0])) {
                uint8_t data[4];
                com_channel_read(connected_channels[0], data, 4);

                number = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
            }
        }

        display_number(segment_pins, digit_pins, number);

        task_yield();
    }
}

void task_count(uint32_t pid) {
    int32_t channel_id = com_channel_request(10);

    while (true) {
        for (uint32_t i = 0; i < 9999; ++i) {
            if (channel_ready_to_write(channel_id)) {
                uint8_t bytes[4];
                bytes[0] = (uint8_t)(i & 0xFF);          // Least significant byte
                bytes[1] = (uint8_t)((i >> 8) & 0xFF);
                bytes[2] = (uint8_t)((i >> 16) & 0xFF);
                bytes[3] = (uint8_t)((i >> 24) & 0xFF);  // Most significant byte
                com_channel_write(channel_id, bytes, 4);
            }

            task_sleep_ms(100);
        }
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

void hash_task(uint32_t pid) {
    bool is_child = (pid != 5);

    if (!is_child) {
        int32_t channel_id = com_channel_request(10);

        if (channel_id < 0) {
            return;
        }

        // make child tasks
        add_task(hash_task, 6, 2);
        add_task(hash_task, 7, 2);

        uint32_t hashes = 0;
        uint8_t update = 0;

        while (true) {

            uint16_t channel_ids[NUM_CHANNELS];
            uint32_t num_connected = get_connected_channels(channel_ids, NUM_CHANNELS);

            // get hash counts from each child
            for (int i = 0; i < num_connected; ++i) {
                if (is_owner_of_channel(i)) {
                    continue;
                }

                if (channel_ready_to_read(i)) {
                    uint8_t bytes[4];
                    com_channel_read(i, bytes, 4);

                    hashes += bytes[0] | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24;
                    update++;
                }
            }

            if (update >= 2) {
                printf("%u\n", hashes);

                // send to screen
                uint8_t bytes[4];
                bytes[0] = (uint8_t)(hashes & 0xFF);          // Least significant byte
                bytes[1] = (uint8_t)((hashes >> 8) & 0xFF);
                bytes[2] = (uint8_t)((hashes >> 16) & 0xFF);
                bytes[3] = (uint8_t)((hashes >> 24) & 0xFF);  // Most significant byte
                com_channel_write(channel_id, bytes, 4);

                hashes = 0;
                update = 0;
            }
            task_sleep_ms(100);
        }
    } else {
        int32_t channel_id = com_channel_request(5);

        if (channel_id < 0) {
            return;
        }

        while (true) {
            absolute_time_t start = get_absolute_time();
            absolute_time_t end = delayed_by_ms(start, 1000);
            char *string = "Hello World!";
            uint8_t result[16];
            uint32_t hashes = 0;

            while (absolute_time_diff_us(get_absolute_time(), end) > 0) {
                md5String(string, result);
                hashes++;
            }

            uint8_t bytes[4];
            bytes[0] = (uint8_t)(hashes & 0xFF);          // Least significant byte
            bytes[1] = (uint8_t)((hashes >> 8) & 0xFF);
            bytes[2] = (uint8_t)((hashes >> 16) & 0xFF);
            bytes[3] = (uint8_t)((hashes >> 24) & 0xFF);  // Most significant byte
            com_channel_write(channel_id, bytes, 4);

            task_yield();
        }
    }
}

int main() {
    stdio_init_all();

    add_task(task_display, 10, 2);
    // add_task(task_count, 9, 2);
    // add_task(stack_overflow_task, 8, 2);
    add_task(hash_task, 5,2);

    start_kernel();

    while (true) {
        tight_loop_contents();
    } // don't return
}