//
// Created by wolfboy on 3/28/2026.
//

#ifndef KELPOS_LITE_MEMORY_PROTECTION_H
#define KELPOS_LITE_MEMORY_PROTECTION_H
#if PICO_RP2040
#include "hardware/regs/m0plus.h"
#include "../scheduler/scheduler_internal.h"
#include "hardware/address_mapped.h"
#define M0PLUS_MPU_BASE_ADDR PPB_BASE

#define MPU_TYPE *(volatile uintptr_t*)(M0PLUS_MPU_BASE_ADDR + M0PLUS_MPU_TYPE_OFFSET)
#define MPU_CTRL *(volatile uintptr_t*)(M0PLUS_MPU_BASE_ADDR + M0PLUS_MPU_CTRL_OFFSET)
#define MPU_RNR  *(volatile uintptr_t*)(M0PLUS_MPU_BASE_ADDR + M0PLUS_MPU_RNR_OFFSET)
#define MPU_RBAR *(volatile uintptr_t*)(M0PLUS_MPU_BASE_ADDR + M0PLUS_MPU_RBAR_OFFSET)
#define MPU_RASR *(volatile uintptr_t*)(M0PLUS_MPU_BASE_ADDR + M0PLUS_MPU_RASR_OFFSET)

#define MPU_BLANKET_ACCESS_REGION_ID 0
#define MPU_FLASH_CODE_EXECUTION_REGION_ID   1
#define MPU_RAM_REGION_ID    2
#define MPU_STACK_GUARD_REGION_ID  3

#define bytes_to_rasr_size(size) (uint32_t)(log((double)size) / log(2.0) - 0.01)
#endif

inline bool mpu_supported() {
#if PICO_RP2040
    return true;
#else
#warning Target System does not have a supported MPU, software protection should be used
    return false;
#endif
}

uint8_t mpu_num_regions();

void mpu_init();

inline void mpu_enable() {
    MPU_CTRL |= M0PLUS_MPU_CTRL_ENABLE_BITS;
}

inline void mpu_disable() {
    MPU_CTRL &= ~M0PLUS_MPU_CTRL_ENABLE_BITS;
}

void mpu_place_stack_guard(task_t* task);

bool mpu_task_overflowed(task_t* task);

#endif //KELPOS_LITE_MEMORY_PROTECTION_H