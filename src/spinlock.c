//
// Created by wolfboy on 8/8/25.
//

#include "spinlock_internal.h"
#include "scheduler_internal.h"

spin_lock_t *spin_lock_scheduler;
spin_lock_t *spin_lock_channel;

int32_t init_spin_locks() {
    spin_lock_claim(SCHEDULER_SPINLOCK_ID);
    spin_lock_claim(CHANNEL_SPINLOCK_ID);

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

inline bool channel_spin_locked() {
    return is_spin_locked(spin_lock_channel);
}

inline uint32_t channel_spin_lock() {
    return spin_lock_blocking(spin_lock_channel);
}

inline void channel_spin_lock_unsafe() {
    spin_lock_unsafe_blocking(spin_lock_channel);
}

inline void channel_spin_unlock(const uint32_t irqs) {
    spin_unlock(spin_lock_channel, irqs);
}

inline void channel_spin_unlock_unsafe() {
    spin_unlock_unsafe(spin_lock_channel);
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

inline bool channel_spin_locked() {
    return false;
}

inline uint32_t channel_spin_lock() {
    return save_and_disable_interrupts();
}

inline void channel_spin_lock_unsafe() {
}

inline void channel_spin_unlock(const uint32_t irqs) {
    restore_interrupts_from_disabled(irqs);
}

inline void channel_spin_unlock_unsafe() {
}
#endif