//
// Created by wolfboy on 3/28/2026.
//

#include "../include/memory/memory_protection.h"
#include <math.h>
#include <stdio.h>

#include "hardware/sync.h"

uint8_t mpu_num_regions() {
#if PICO_RP2040
    uint8_t num_regions = (MPU_TYPE & M0PLUS_MPU_TYPE_DREGION_BITS) >> 8;
    return num_regions;
#else
    return 0;
#endif
}

void mpu_enable() {
    __dsb();
    MPU_CTRL |= M0PLUS_MPU_CTRL_ENABLE_BITS;
    __isb();
}

void mpu_disable() {
    __dsb();
    MPU_CTRL &= ~M0PLUS_MPU_CTRL_ENABLE_BITS;
    __isb();
}

void mpu_init() {
#if PICO_RP2040
    uint32_t attrs = 0;
    uint32_t sub_regions = 0;
    uint32_t size = 0;

    // we're trying to replicate the default memory map described in
    // Table 82 of the RP2040 documentation

    // select the region
    MPU_RNR = M0PLUS_MPU_RNR_REGION_BITS & MPU_DEFAULT_DEVICE_REGION_ID;

    // set region address to the start the address space
    MPU_RBAR = 0;

    // set primary region size and attributes
    //           ↓ (XN bit) allow execution
    attrs = 0b0000001100000101 << 16;
    //             ↑↑↑ (AP bits) full access

    sub_regions = 0b00000000 << 8; // all subregions enabled

    size = 31 << 1; // set approximate size

    MPU_RASR = (M0PLUS_MPU_RASR_ATTRS_BITS & attrs) |
        (M0PLUS_MPU_RASR_SRD_BITS & sub_regions) |
        (M0PLUS_MPU_RASR_SIZE_BITS & size) |
        M0PLUS_MPU_RASR_ENABLE_BITS;

    // select the region
    MPU_RNR = M0PLUS_MPU_RNR_REGION_BITS & MPU_DEFAULT_NORMAL_REGION_ID;

    // set region address to the start the address space
    MPU_RBAR = 0;

    // set primary region size and attributes
    //           ↓ (XN bit) allow execution
    attrs = 0b0000001100000111 << 16;
    //             ↑↑↑ (AP bits) full access

    sub_regions = 0b00000000 << 8; // all subregions enabled

    size = 29 << 1; // set approximate size

    MPU_RASR = (M0PLUS_MPU_RASR_ATTRS_BITS & attrs) |
        (M0PLUS_MPU_RASR_SRD_BITS & sub_regions) |
        (M0PLUS_MPU_RASR_SIZE_BITS & size) |
        M0PLUS_MPU_RASR_ENABLE_BITS;

    // enable PRIVDEFENA and MPU
    __dsb();
    MPU_CTRL = M0PLUS_MPU_CTRL_PRIVDEFENA_BITS | M0PLUS_MPU_CTRL_ENABLE_BITS;
    __isb();
#endif
}

void mpu_place_stack_guard(task_t* task) {
#if PICO_RP2040
    mpu_disable();

    // place restrictive stack guard

    // select the region
    MPU_RNR = M0PLUS_MPU_RNR_REGION_BITS & MPU_STACK_GUARD_REGION_ID;

    // set secondary region address to bottom of stack
    uint32_t region_addr = (uint32_t)task->stack;
    MPU_RBAR = region_addr & M0PLUS_MPU_RBAR_ADDR_BITS;

    // set secondary region size and attributes
    //                    ↓ (XN bit) allow execution
    uint32_t attrs = 0b0000000100000111 << 16;
    //                      ↑↑↑ (AP bits) privileged can access but not unprivileged

    uint32_t sub_regions = 0b11111101 << 8;

    uint32_t size = 7 << 1; // set approximate size

    MPU_RASR = (M0PLUS_MPU_RASR_ATTRS_BITS & attrs) |
        (M0PLUS_MPU_RASR_SRD_BITS & sub_regions) |
        (M0PLUS_MPU_RASR_SIZE_BITS & size) |
        M0PLUS_MPU_RASR_ENABLE_BITS;

    scheduler_t *scheduler = get_scheduler();
    scheduler->mpu_region_addr = region_addr;
    scheduler->mpu_region_size = 64;

    mpu_enable();
#endif
}

bool mpu_task_overflowed(task_t* task, uint32_t psp) {
    #if PICO_RP2040
        return ((uint32_t)task->stack_pointer - psp) < STACK_OVERFLOW_THRESHOLD;
    #endif
}
