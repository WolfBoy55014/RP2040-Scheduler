//
// Created by wolfboy on 2/16/2026.
//

#include <stdio.h>

#include "governor.h"
#include "kernel_config.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "pico/stdio.h"

const uint32_t governor_frequencies[] = GOVERNOR_FREQUENCIES;
const enum vreg_voltage governor_voltages[] = GOVERNOR_VOLTAGES;

uint8_t current_power_mode = GOVERNOR_BALANCED;
uint8_t target_utilization = GOVERNOR_TARGET_BALANCED;

uint32_t current_freq = GOVERNOR_DEFAULT_FREQ;

void governor_set_mode(uint8_t power_mode) {
    current_power_mode = power_mode;

    switch (power_mode) {
    case (GOVERNOR_PERFORMANCE):
        target_utilization = GOVERNOR_TARGET_PERFORMANCE;
        break;
    case (GOVERNOR_BALANCED):
        target_utilization = GOVERNOR_TARGET_BALANCED;
        break;
    case (GOVERNOR_POWER_SAVE):
        target_utilization = GOVERNOR_TARGET_POWER_SAVE;
        break;
    default:
        target_utilization = GOVERNOR_TARGET_BALANCED;
        break;
    }
}

uint8_t governor_get_mode() {
    return current_power_mode;
}

void governor_init() {
    current_power_mode = GOVERNOR_BALANCED;
    target_utilization = GOVERNOR_TARGET_BALANCED;
    current_freq = GOVERNOR_DEFAULT_FREQ;
}

void governor_update() {
    uint32_t new_freq = current_freq;

    uint8_t core_usage = 0;
    for (int c = 0; c < CORE_COUNT; c++) {
        core_usage += get_core_usage(c);
    }
    core_usage /= CORE_COUNT;

    // printf(">core_usage: %u\r\n", core_usage);

    if (core_usage > target_utilization + GOVERNOR_TARGET_TOLERANCE) {
        if (new_freq < 11) {
            new_freq++;
        }
    }
    else if (core_usage < target_utilization - GOVERNOR_TARGET_TOLERANCE) {
        if (new_freq > 0) {
            new_freq--;
        }
    }

    if (new_freq != current_freq) {
        enum vreg_voltage target_voltage = governor_voltages[new_freq];

        // increase voltage before increasing frequency
        if (new_freq > current_freq) {
            vreg_set_voltage(target_voltage);
            busy_wait_us(100);  // Let voltage stabilize
        }

        if (set_sys_clock_khz(governor_frequencies[new_freq], false)) {
            vreg_set_voltage(governor_voltages[new_freq]);

            // decrease voltage after decreasing frequency
            if (new_freq < current_freq) {
                busy_wait_us(10);  // Let clock stabilize
                vreg_set_voltage(target_voltage);
            }

            current_freq = new_freq;

            refresh_systick_all_cores();
            stdio_init_all();

            busy_wait_us(10);

            // printf("Successfully set system clock frequency to %lu\n", governor_frequencies[current_freq]);
        }
    }
}