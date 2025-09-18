#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <stdio.h>
#include <pico/stdlib.h>

#include "kernel_config.h"
#include "channel.h"
#include "md5.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "spinlock_internal.h"
#include "pico/runtime_init.h"

void spin(const uint32_t n) {
    switch (n % 4) {
        case 0:
            printf("\b|");
            break;

        case 1:
            printf("\b/");
            break;

        case 2:
            printf("\b-");
            break;

        case 3:
            printf("\b\\");
            break;
        default: ;
    }
}

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
uint32_t recursively_check_stack(const uint32_t a, const uint32_t b, uint16_t depth) {
    const uint32_t c = a + b;

    spin(depth / 16);

    if (depth <= 0) {
        return c;
    }

    task_sleep_ms(2);
    const uint32_t d = recursively_check_stack(b, c, depth - 1);
    task_sleep_ms(2);

    spin(depth / 8);

    // validate result
    if (d - c == b) {
        // correct
        return c;
    } else {
        // error
        printf("\nMemory corruption at depth %u (this is counted from the end)\n", depth);
        task_end(-1);
    }

    return 0;
}

void stack_protection_test_task(uint32_t pid) {
    // wait to connect to `unit_test_task`
    uint16_t channels[NUM_CHANNELS];
    while (get_connected_channels(channels, NUM_CHANNELS) <= 0) {task_yield();}
    uint16_t cid = channels[0];

    // get length of test
    uint8_t bytes[CHANNEL_SIZE];
    while (!channel_ready_to_read(cid)) {task_yield();}
    com_channel_read(cid, bytes, CHANNEL_SIZE);

    uint32_t test_length = bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
    printf("Doing Stack Protection Test of length %u |", test_length);

    recursively_check_stack(0, 1, test_length);

    printf("\n" "\e[0;32m" "SUCCESS" "\e[0m" "\n");
}

void unit_test_task(uint32_t pid) {
    printf("\nStarting Unit Tests\n");
    task_sleep_ms(1000);

    printf("Testing Stack Management\n");
    task_sleep_ms(1000);

    printf("Testing Overflow Protection\n");
    const uint32_t protection_test_pid = pid + 1;
    const uint32_t protection_test_length = 1024;

    add_task(stack_protection_test_task, protection_test_pid, 7);

    const uint32_t protection_test_cid = com_channel_request(protection_test_pid);
    uint8_t bytes[CHANNEL_SIZE];
    bytes[0] = protection_test_length >> 24;
    bytes[1] = protection_test_length >> 16;
    bytes[2] = protection_test_length >> 8;
    bytes[3] = protection_test_length;
    com_channel_write(protection_test_cid, bytes, CHANNEL_SIZE);

    while(true) {
        task_yield();
    }
}

void monitor_task(uint32_t pid) {
    const uint8_t length = 20;

    while (true) {
        printf("========= System Report =========\n");

        for (uint8_t c = 0; c < CORE_COUNT; c++) {
            printf("Core %u: [", c);

            uint8_t usage = get_core_usage(c);
            for (int u = 0; u < 100; u += 100 / length) {
                if (u <= usage) {
                    printf("█");
                } else {
                    printf(" ");
                }
            }

            printf("] %u%%\n", usage);
        }

        printf("\n        --- CPU Usage ---\n");

        for (uint32_t t = 0; t < MAX_TASKS; t++) {
            task_t *task = &tasks[t];

            if (task->state == TASK_FREE) {
                continue;
            }

            printf("Task %u: [", task->id);

            uint8_t usage = task->cpu_usage;
            for (int u = 0; u < 100; u += 100 / length) {
                if (u <= usage) {
                    printf("░");
                } else {
                    printf(" ");
                }
            }

            printf("] %u%%\n", usage);
        }

        printf("\n       --- Stack Usage ---\n");

        for (uint32_t t = 0; t < MAX_TASKS; t++) {
            task_t *task = &tasks[t];

            if (task->state == TASK_FREE) {
                continue;
            }

            printf("Task %u: [", task->id);

            uint8_t usage = task->stack_usage;
            for (int u = 0; u < 100; u += 100 / length) {
                if (u <= usage) {
                    printf("░");
                } else {
                    printf(" ");
                }
            }

            printf("] %u%% (%u bytes)\n", usage, task->stack_size * 4);
        }

        printf("=================================\n\n");

        task_sleep_ms(1000);
    }
}

int main() {
    stdio_init_all();

    // add_task(task_display, 10, 2);
    // add_task(monitor_task, 11, 8);
    add_task(unit_test_task, 4, 7);

    start_kernel();

    while (true) {
        tight_loop_contents();
    } // don't return
}