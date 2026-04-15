//
// Created by wolfboy on 3/28/2026.
//

#include "../include/memory/memory_protection.h"
#include <math.h>
#include <stdio.h>

uint8_t mpu_num_regions() {
#if PICO_RP2040
    uint8_t num_regions = (MPU_TYPE & M0PLUS_MPU_TYPE_DREGION_BITS) >> 8;
    return num_regions;
#else
    return 0;
#endif
}

void mpu_init() {
#if PICO_RP2040
    uint32_t attrs = 0;
    uint32_t sub_regions = 0;
    uint32_t size = 0;

    // allow access everywhere

    // select the region
    MPU_RNR = M0PLUS_MPU_RNR_REGION_BITS & MPU_BLANKET_ACCESS_REGION_ID;

    // set primary region address to the start the address space
    MPU_RBAR = 0;

    // set primary region size and attributes
    //           ↓ (XN bit) allow execution
    attrs = 0b0000001100000000 << 16;
    //             ↑↑↑ (AP bits) full access

    sub_regions = 0b00000000 << 8; // all subregions enabled

    size = 31 << 1; // set approximate size

    MPU_RASR = (M0PLUS_MPU_RASR_ATTRS_BITS & attrs) |
        (M0PLUS_MPU_RASR_SRD_BITS & sub_regions) |
        (M0PLUS_MPU_RASR_SIZE_BITS & size) |
        M0PLUS_MPU_RASR_ENABLE_BITS;

    // prevent execution in ram

    // select the region
    MPU_RNR = M0PLUS_MPU_RNR_REGION_BITS & MPU_RAM_REGION_ID;

    // set primary region address to the start of ram
    MPU_RBAR = SRAM_BASE & M0PLUS_MPU_RBAR_ADDR_BITS;

    // set primary region size and attributes
    //           ↓ (XN bit) allow execution
    attrs = 0b0000001100000000 << 16;
    //             ↑↑↑ (AP bits) full access

    sub_regions = 0b00000000 << 8; // all subregions enabled

    size = bytes_to_rasr_size(SRAM_END - SRAM_BASE) << 1; // set approximate size

    MPU_RASR = (M0PLUS_MPU_RASR_ATTRS_BITS & attrs) |
        (M0PLUS_MPU_RASR_SRD_BITS & sub_regions) |
        (M0PLUS_MPU_RASR_SIZE_BITS & size) |
        M0PLUS_MPU_RASR_ENABLE_BITS;

    // permit unprivileged code to execute from flash

    // select the region
    MPU_RNR = M0PLUS_MPU_RNR_REGION_BITS & MPU_FLASH_CODE_EXECUTION_REGION_ID;

    // set region address to the beginning of flash
    uint32_t region_addr = XIP_BASE;
    MPU_RBAR = region_addr & M0PLUS_MPU_RBAR_ADDR_BITS;

    // set primary region size and attributes
    //           ↓ (XN bit) allow execution
    attrs = 0b0000001000000000 << 16;
    //             ↑↑↑ (AP bits) unprivileged read-only

    sub_regions = 0b00000000 << 8; // all subregions enabled

    size = bytes_to_rasr_size(PICO_FLASH_SIZE_BYTES) << 1; // set approximate size
    // don't worry about going over, there is a big gap
    // between flash and ram

    MPU_RASR = (M0PLUS_MPU_RASR_ATTRS_BITS & attrs) |
        (M0PLUS_MPU_RASR_SRD_BITS & sub_regions) |
        (M0PLUS_MPU_RASR_SIZE_BITS & size) |
        M0PLUS_MPU_RASR_ENABLE_BITS;


    // enable PRIVDEFENA, HFNMIENA and MPU
    MPU_CTRL = M0PLUS_MPU_CTRL_PRIVDEFENA_BITS | M0PLUS_MPU_CTRL_ENABLE_BITS;
#endif
}

void mpu_place_stack_guard(task_t* task) {
#if PICO_RP2040
    mpu_disable();

    // place restrictive stack guard

    // select the region
    MPU_RNR = M0PLUS_MPU_RNR_REGION_BITS & MPU_STACK_GUARD_REGION_ID;

    // set secondary region address to bottom of stack
    uint32_t region_addr = (uint32_t)task->stack - 256;
    MPU_RBAR = region_addr & M0PLUS_MPU_RBAR_ADDR_BITS;

    // set secondary region size and attributes
    //                    ↓ (XN bit) no execution
    uint32_t attrs = 0b0001000100000000 << 16;
    //                      ↑↑↑ (AP bits) privileged can access but not unprivileged

    uint32_t sub_regions = 0b00000000 << 8; // only highest subregion is enabled

    uint32_t size = 7 << 1; // set approximate size

    MPU_RASR = (M0PLUS_MPU_RASR_ATTRS_BITS & attrs) |
        (M0PLUS_MPU_RASR_SRD_BITS & sub_regions) |
        (M0PLUS_MPU_RASR_SIZE_BITS & size) |
        M0PLUS_MPU_RASR_ENABLE_BITS;

    mpu_enable();
#endif
}

bool mpu_task_overflowed(task_t* task) {
    #if PICO_RP2040
        return (task->stack_pointer - task->stack <= 32);
    #endif
}
