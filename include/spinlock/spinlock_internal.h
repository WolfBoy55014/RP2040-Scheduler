//
// Created by wolfboy on 8/8/25.
//

#ifndef SPINLOCK_INTERNAL_H
#define SPINLOCK_INTERNAL_H

#include "hardware/sync.h"
#include "kernel_config.h"

extern spin_lock_t *spin_lock_scheduler;
extern spin_lock_t *spin_lock_channel;

extern spin_lock_t *channel_spin_locks[NUM_CHANNEL_SPINLOCKS];

void spin_locks_init();

bool scheduler_spin_locked();

uint32_t scheduler_spin_lock();

void scheduler_spin_lock_unsafe();

void scheduler_spin_unlock(uint32_t irqs);

void scheduler_spin_unlock_unsafe();

bool global_channel_spin_locked();

uint32_t global_channel_spin_lock();

void global_channel_spin_lock_unsafe();

void global_channel_spin_unlock(uint32_t irqs);

void global_channel_spin_unlock_unsafe();

bool channel_spin_locked(uint16_t channel_id);

uint32_t channel_spin_lock(uint16_t channel_id);

void channel_spin_lock_unsafe(uint16_t channel_id);

void channel_spin_unlock(uint16_t channel_id, uint32_t irqs);

void channel_spin_unlock_unsafe(uint16_t channel_id);

#endif //SPINLOCK_INTERNAL_H
