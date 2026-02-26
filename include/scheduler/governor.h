//
// Created by wolfboy on 2/16/2026.
//

#ifndef KELPOS_LITE_GOVERNOR_H
#define KELPOS_LITE_GOVERNOR_H

#include "kernel_config.h"

#if USE_GOVERNOR == 1

#include <stdint.h>
#include "hardware/vreg.h"

#define GOVERNOR_FREQUENCIES    {48000, 64000, 72000, 80000, 96000, 110000, 125000, 133000, 150000, 175000, 200000, 240000}
#define GOVERNOR_VOLTAGES       {VREG_VOLTAGE_0_90, VREG_VOLTAGE_0_90, VREG_VOLTAGE_0_90, VREG_VOLTAGE_0_95, VREG_VOLTAGE_0_95, VREG_VOLTAGE_1_10, VREG_VOLTAGE_1_10, VREG_VOLTAGE_1_10, VREG_VOLTAGE_1_15, VREG_VOLTAGE_1_15, VREG_VOLTAGE_1_20, VREG_VOLTAGE_1_20}
#define GOVERNOR_DEFAULT_FREQ   5       // should be the index of the frequency the cpu
                                        // normally runs at

#define GOVERNOR_TARGET_PERFORMANCE 25  // target cpu utilization for governor to aim for while in performance mode
#define GOVERNOR_TARGET_BALANCED    50  // target cpu utilization for governor to aim for normally
#define GOVERNOR_TARGET_POWER_SAVE  80  // target cpu utilization for governor to aim for while trying to same power
#define GOVERNOR_TARGET_TOLERANCE   5   // the governor will only adjust the cpu frequency is
                                        // this many percent above or below the target utilization

#define GOVERNOR_PERFORMANCE    2
#define GOVERNOR_BALANCED       1
#define GOVERNOR_POWER_SAVE     0

extern const uint32_t governor_frequencies[];
extern const enum vreg_voltage governor_voltages[];

/**
 * Set the power mode
 */
void governor_set_mode(uint8_t power_mode);

/**
 * Get the current power mode
 */
uint8_t governor_get_mode();

/**
 * Initialize the governor
 */
void governor_init();

/**
 * Iterate the governor
 */
void governor_update();

#endif
#endif //KELPOS_LITE_GOVERNOR_H