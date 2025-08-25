//
// Created by wolfboy on 8/8/25.
//

#ifndef SPINLOCK_INTERNAL_H
#define SPINLOCK_INTERNAL_H

#include "hardware/sync.h"

extern spin_lock_t *spin_lock_scheduler;
extern spin_lock_t *spin_lock_channel;

int32_t init_spin_locks();

bool scheduler_spin_locked();

uint32_t scheduler_spin_lock();

void scheduler_spin_lock_unsafe();

void scheduler_spin_unlock(uint32_t irqs);

void scheduler_spin_unlock_unsafe();

bool channel_spin_locked();

uint32_t channel_spin_lock();

void channel_spin_lock_unsafe();

void channel_spin_unlock(uint32_t irqs);

void channel_spin_unlock_unsafe();

#endif //SPINLOCK_INTERNAL_H
