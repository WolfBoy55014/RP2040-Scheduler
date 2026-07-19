//
// Created by wolfboy on 8/8/25.
//

#include "spinlock_internal.h"
#include "scheduler_internal.h"

spin_lock_t *spin_lock_scheduler;
spin_lock_t *spin_lock_channel;

spin_lock_t *channel_spin_locks[NUM_CHANNEL_SPINLOCKS];

void spin_locks_init() {
    spin_lock_claim(SCHEDULER_SPINLOCK_ID);
    spin_lock_claim(CHANNEL_SPINLOCK_ID);

    for (uint32_t i = 0; i < NUM_CHANNEL_SPINLOCKS; i++) {
        int lock_num = spin_lock_claim_unused(true);
        channel_spin_locks[i] = spin_lock_init(lock_num);
    }

    spin_lock_scheduler = spin_lock_init(SCHEDULER_SPINLOCK_ID);
    spin_lock_channel = spin_lock_init(CHANNEL_SPINLOCK_ID);
}

#if CORE_COUNT > 1
inline bool scheduler_spin_locked() {
    return is_spin_locked(spin_lock_scheduler);
}

inline uint32_t scheduler_spin_lock() {
    return spin_lock_blocking(spin_lock_scheduler);
}

inline void scheduler_spin_lock_unsafe() {
    spin_lock_unsafe_blocking(spin_lock_scheduler);
}

inline void scheduler_spin_unlock(const uint32_t irqs) {
    spin_unlock(spin_lock_scheduler, irqs);
}

inline void scheduler_spin_unlock_unsafe() {
    spin_unlock_unsafe(spin_lock_scheduler);
}

inline bool global_channel_spin_locked() {
    return is_spin_locked(spin_lock_channel);
}

inline uint32_t global_channel_spin_lock() {
    return spin_lock_blocking(spin_lock_channel);
}

inline void global_channel_spin_lock_unsafe() {
    spin_lock_unsafe_blocking(spin_lock_channel);
}

inline void global_channel_spin_unlock(const uint32_t irqs) {
    spin_unlock(spin_lock_channel, irqs);
}

inline void global_channel_spin_unlock_unsafe() {
    spin_unlock_unsafe(spin_lock_channel);
}

inline spin_lock_t *get_channel_spin_lock(uint16_t channel_id) {
    return channel_spin_locks[channel_id % NUM_CHANNEL_SPINLOCKS];
}

inline bool channel_spin_locked(uint16_t channel_id) {
    spin_lock_t *lock = get_channel_spin_lock(channel_id);
    return is_spin_locked(lock);
}

inline uint32_t channel_spin_lock(uint16_t channel_id) {
    spin_lock_t *lock = get_channel_spin_lock(channel_id);
    return spin_lock_blocking(lock);
}

inline void channel_spin_lock_unsafe(uint16_t channel_id) {
    spin_lock_t *lock = get_channel_spin_lock(channel_id);
    spin_lock_unsafe_blocking(lock);
}

inline void channel_spin_unlock(uint16_t channel_id, const uint32_t irqs) {
    spin_lock_t *lock = get_channel_spin_lock(channel_id);
    spin_unlock(lock, irqs);
}

inline void channel_spin_unlock_unsafe(uint16_t channel_id) {
    spin_lock_t *lock = get_channel_spin_lock(channel_id);
    spin_unlock_unsafe(lock);
}
#else
inline bool scheduler_spin_locked() {
    return false;
}

inline uint32_t scheduler_spin_lock() {
    return save_and_disable_interrupts();
}

inline void scheduler_spin_lock_unsafe() {
}

inline void scheduler_spin_unlock(const uint32_t irqs) {
    restore_interrupts_from_disabled(irqs);
}

inline void scheduler_spin_unlock_unsafe() {
}

inline bool global_channel_spin_locked() {
    return false;
}

inline uint32_t global_channel_spin_lock() {
    return save_and_disable_interrupts();
}

inline void global_channel_spin_lock_unsafe() {
}

inline void global_channel_spin_unlock(const uint32_t irqs) {
    restore_interrupts_from_disabled(irqs);
}

inline void global_channel_spin_unlock_unsafe() {
}

inline bool channel_spin_locked(uint16_t channel_id) {
    return false;
}

inline uint32_t channel_spin_lock(uint16_t channel_id) {
    return save_and_disable_interrupts();
}

inline void channel_spin_lock_unsafe(uint16_t channel_id) {
}

inline void channel_spin_unlock(uint16_t channel_id, const uint32_t irqs) {
    restore_interrupts_from_disabled(irqs);
}

inline void channel_spin_unlock_unsafe(uint16_t channel_id) {
}
#endif