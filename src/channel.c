//
// Created by wolfboy on 7/29/25.
//

#include "channel_internal.h"
#include "channel.h"

#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "scheduler_internal.h"
#include "spinlock_internal.h"
#include "hardware/sync.h"

com_channel_t com_channels[NUM_CHANNELS];

kelp_error_t channel_garbage_collect() {
    // go through all the channels,
    // and check if they can be automatically removed

    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        channel_spin_lock_unsafe(c);
        com_channel_t* channel = &com_channels[c];

        if (channel->state == CHANNEL_FREE) {
            channel_spin_unlock_unsafe(c);
            continue;
        }

        // remove it if it's owner no longer exists
        if (!task_exists(channel->owner->id)) {
            channel_spin_unlock_unsafe(c);
            com_channel_free(c);
            continue;
        }

        // remove it if it's set to auto free
        if (channel->can_auto_free) {
            if (channel->inactivity_cooldown <= 0) {
                channel_spin_unlock_unsafe(c);
                com_channel_free(c);
                continue;
            }

            channel->inactivity_cooldown -= CHANNEL_GARBAGE_COLLECT_PERIOD;
        }
        channel_spin_unlock_unsafe(c);
    }
    return KELP_OK;
}

kelp_error_t init_channels() {
    uint32_t saved_irq = global_channel_spin_lock();

    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        channel_spin_lock_unsafe(c);
        // alloc memory for the fifos
        com_channel_t* channel = &com_channels[c];
        channel->state = CHANNEL_FREE;

        void* rx_memory = malloc(sizeof(uint8_t) * CHANNEL_SIZE);
        if (rx_memory == NULL) {
            channel_spin_unlock_unsafe(c);
            global_channel_spin_unlock(saved_irq);
            return KELP_MEMORY;
        }

        void* tx_memory = malloc(sizeof(uint8_t) * CHANNEL_SIZE);
        if (tx_memory == NULL) {
            free(rx_memory);
            channel_spin_unlock_unsafe(c);
            global_channel_spin_unlock(saved_irq);
            return KELP_MEMORY;
        }

        channel->fifo_rx.bytes = (uint8_t*) rx_memory;
        channel->fifo_tx.bytes = (uint8_t*) tx_memory;
        channel_spin_unlock_unsafe(c);
    }
    global_channel_spin_unlock(saved_irq);
    return KELP_OK;
}

bool is_owner_of_channel_no_lock(const uint16_t channel_id) {
    if (channel_id >= NUM_CHANNELS) {
        return false;
    }

    if (is_privileged()) {
        return true;
    }

    com_channel_t* channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        return false;
    }

    return get_current_task() == channel->owner;
}

bool is_owner_of_channel(const uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock(channel_id);

    bool is_owner = is_owner_of_channel_no_lock(channel_id);

    channel_spin_unlock(channel_id, saved_irq);
    return is_owner;
}

bool is_connected_to_channel_no_lock(const uint16_t channel_id) {
    if (channel_id >= NUM_CHANNELS) {
        return false;
    }

    if (is_privileged()) {
        return true;
    }

    com_channel_t* channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        return false;
    }

    task_t* current_task = get_current_task();
    return (current_task == channel->owner || current_task == channel->partner);
}

bool is_connected_to_channel(const uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock(channel_id);

    bool is_connected = is_connected_to_channel_no_lock(channel_id);

    channel_spin_unlock(channel_id, saved_irq);
    return is_connected;
}

kelp_error_t get_connected_channels_no_lock(uint16_t* channel_ids, uint16_t* num_connected, uint16_t size) {
    *num_connected = 0;

    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        // the no_lock isn't needed as they need different locks
        // luckily, the correct order is still maintained
        if (is_connected_to_channel(c)) {
            if (*num_connected >= size) {
                return KELP_TOO_BIG;
            }

            channel_ids[*num_connected] = c;
            *num_connected = *num_connected + 1;
        }
    }

    return KELP_OK;
}

// TODO: Optimize with cacheing
kelp_error_t get_connected_channels(uint16_t* channel_ids, uint16_t* num_connected, uint16_t size) {
    const uint32_t saved_irq = global_channel_spin_lock();

    kelp_error_t error = get_connected_channels_no_lock(channel_ids, num_connected, size);

    global_channel_spin_unlock(saved_irq);
    return error;
}

uint32_t get_channel_partner_pid(uint16_t channel_id) {
    // whole thing is now protected
    uint32_t saved_irq = channel_spin_lock(channel_id);

    if (is_connected_to_channel_no_lock(channel_id)) {
        if (is_owner_of_channel_no_lock(channel_id)) {
            channel_spin_unlock(channel_id, saved_irq);
            return com_channels[channel_id].partner->id;
        } else {
            channel_spin_unlock(channel_id, saved_irq);
            return com_channels[channel_id].owner->id;
        }
    }

    channel_spin_unlock(channel_id, saved_irq);
    return 0;
}

kelp_error_t com_channel_request(uint32_t with_pid, bool autoFree, uint16_t* channel_id) {
    const uint32_t saved_irq = scheduler_spin_lock();

    task_t* with = NULL;
    com_channel_t* channel = NULL;

    // check if `with_pid` matches the current (callee's) pid
    task_t* current_task = get_current_task();
    if (current_task->id == with_pid) {
        scheduler_spin_unlock(saved_irq);
        return KELP_INVALID_ID;
    }

    // find task with specified pid
    for (uint32_t t = 0; t < num_tasks; t++) {
        if (tasks[t].id == with_pid) {
            with = &tasks[t];
            break;
        }
    }
    scheduler_spin_unlock_unsafe();
    global_channel_spin_lock_unsafe();

    // if we could not find the task with `with_pid` return
    if (with == NULL) {
        global_channel_spin_unlock(saved_irq);
        return KELP_INVALID_ID;
    }

    // check if a channel already connects these two channels
    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        channel_spin_lock_unsafe(c);
        channel = &com_channels[c];
        if (channel->state == CHANNEL_CONNECTED) {
            if (channel->owner->id == current_task->id &&
                channel->partner->id == with_pid) {
                *channel_id = c; // remind them they already have this one
                channel_spin_unlock_unsafe(c);
                global_channel_spin_unlock(saved_irq);
                return KELP_OK;
            }
        }
        channel_spin_unlock_unsafe(c);
    }


    // check for free channels
    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        channel_spin_lock_unsafe(c);
        if (com_channels[c].state == CHANNEL_FREE) {
            channel = &com_channels[c];
            *channel_id = c;
            channel_spin_unlock_unsafe(c);
            break;
        }
        channel_spin_unlock_unsafe(c);
    }

    // if there are none, return
    if (channel == NULL) {
        global_channel_spin_unlock(saved_irq);
        return KELP_NONE_FREE;
    }

    channel_spin_lock_unsafe(*channel_id);
    channel->state = CHANNEL_ALLOCATED;

    channel->owner = current_task;
    channel->partner = with;

    for (uint32_t i = 0; i < CHANNEL_SIZE; ++i) {
        channel->fifo_rx.bytes[i] = 0;
        channel->fifo_tx.bytes[i] = 0;
    }

    channel->fifo_rx.count = 0;
    channel->fifo_tx.count = 0;

    channel->fifo_rx.full = 0;
    channel->fifo_tx.full = 0;

    channel->state = CHANNEL_CONNECTED;
    channel->can_auto_free = autoFree;
    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock_unsafe(*channel_id);
    global_channel_spin_unlock(saved_irq);
    return KELP_OK;
}

kelp_error_t com_channel_request_blocking(uint32_t with_pid, bool autoFree, uint16_t* channel_id) {
    kelp_error_t error = KELP_NONE_FREE;
    for (uint16_t t = 0; t < CHANNEL_BLOCKING_TIMEOUT_MS; t++) {
        error = com_channel_request(with_pid, autoFree, channel_id);
        if (error == KELP_OK) {
            break;
        }
        task_sleep_ms(1);
    }

    return error;
}

kelp_error_t com_channel_free(uint16_t channel_id) {
    const uint32_t saved_irq = global_channel_spin_lock();
    channel_spin_lock_unsafe(channel_id);

    // check if channel exists
    if (channel_id >= NUM_CHANNELS) {
        channel_spin_unlock_unsafe(channel_id);
        global_channel_spin_unlock(saved_irq);
        return KELP_INVALID_ID;
    }

    // check if the current task is owner of the channel
    if (!is_owner_of_channel_no_lock(channel_id)) {
        channel_spin_unlock_unsafe(channel_id);
        global_channel_spin_unlock(saved_irq);
        return KELP_NOT_OWNER;
    }

    com_channel_t* channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        channel_spin_unlock_unsafe(channel_id);
        global_channel_spin_unlock(saved_irq);
        return KELP_UNALLOCATED;
    }

    // empty channel of contents to prevent spying
    memset(channel->fifo_rx.bytes, 0, CHANNEL_SIZE);
    memset(channel->fifo_tx.bytes, 0, CHANNEL_SIZE);

    channel->fifo_rx.count = 0;
    channel->fifo_tx.count = 0;

    channel->fifo_rx.full = 0;
    channel->fifo_tx.full = 0;

    // free channel
    channel->state = CHANNEL_FREE;
    channel->can_auto_free = false;
    channel->inactivity_cooldown = 0;

    channel_spin_unlock_unsafe(channel_id);
    global_channel_spin_unlock(saved_irq);
    return KELP_OK;
}

bool is_channel_ready_to_write(const uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock(channel_id);

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(channel_id, saved_irq);
        return false;
    }

    com_channel_t* channel = &com_channels[channel_id];
    channel_fifo_t* fifo;

    if (is_owner_of_channel_no_lock(channel_id)) {
        fifo = &channel->fifo_tx;
    }
    else {
        fifo = &channel->fifo_rx;
    }

    if (fifo->full) {
        channel_spin_unlock(channel_id, saved_irq);
        return false;
    }

    channel_spin_unlock(channel_id, saved_irq);
    return true;
}

kelp_error_t com_channel_write(uint16_t channel_id, const uint8_t* bytes, uint16_t size) {
    const uint32_t saved_irq = channel_spin_lock(channel_id);

    if (size > CHANNEL_SIZE) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_TOO_BIG;
    }

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_NOT_CONNECTED;
    }

    com_channel_t* channel = &com_channels[channel_id];
    channel_fifo_t* fifo;

    if (is_owner_of_channel_no_lock(channel_id)) {
        fifo = &channel->fifo_tx;
    }
    else {
        fifo = &channel->fifo_rx;
    }

    if (fifo->full) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_CHANNEL_FULL;
    }

    memcpy(fifo->bytes, bytes, size);

    fifo->count = size;
    fifo->full = 1;
    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(channel_id, saved_irq);
    return KELP_OK;
}

kelp_error_t com_channel_write_blocking(uint16_t channel_id, const uint8_t* bytes, uint16_t size) {
    com_channel_wait_until_writable(channel_id);

    return com_channel_write(channel_id, bytes, size);
}

bool is_channel_ready_to_read(const uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock(channel_id);

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(channel_id, saved_irq);
        return false;
    }

    com_channel_t* channel = &com_channels[channel_id];
    channel_fifo_t* fifo;

    if (is_owner_of_channel_no_lock(channel_id)) {
        fifo = &channel->fifo_rx;
    }
    else {
        fifo = &channel->fifo_tx;
    }

    if (!fifo->full) {
        channel_spin_unlock(channel_id, saved_irq);
        return false;
    }

    channel_spin_unlock(channel_id, saved_irq);
    return true;
}

kelp_error_t com_channel_read(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size) {
    uint32_t saved_irq = channel_spin_lock(channel_id);

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_NOT_CONNECTED;
    }

    com_channel_t* channel = &com_channels[channel_id];
    channel_fifo_t* fifo;

    if (is_owner_of_channel_no_lock(channel_id)) {
        fifo = &channel->fifo_rx;
    }
    else {
        fifo = &channel->fifo_tx;
    }

    if (!fifo->full) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_CHANNEL_EMPTY;
    }

    uint32_t fifo_count = fifo->count;

    if (size < fifo_count) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_TOO_BIG;
    }

    memcpy(buffer, fifo->bytes, fifo_count);

    *read = fifo_count;
    fifo->count = 0;
    fifo->full = 0;
    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(channel_id, saved_irq);
    return KELP_OK;
}

kelp_error_t com_channel_read_blocking(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size) {
    com_channel_wait_until_readable(channel_id);

    return com_channel_read(channel_id, buffer, read, size);
}

kelp_error_t com_channel_read_no_reset(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size) {
    uint32_t saved_irq = channel_spin_lock(channel_id);

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_NOT_CONNECTED;
    }

    com_channel_t* channel = &com_channels[channel_id];
    channel_fifo_t* fifo;

    if (is_owner_of_channel_no_lock(channel_id)) {
        fifo = &channel->fifo_rx;
    }
    else {
        fifo = &channel->fifo_tx;
    }

    if (!fifo->full) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_CHANNEL_EMPTY;
    }

    uint32_t fifo_count = fifo->count;

    if (size < fifo_count) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_TOO_BIG;
    }

    for (int b = 0; b < fifo_count; b++) {
        buffer[b] = fifo->bytes[b];
    }

    *read = fifo_count;
    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(channel_id, saved_irq);
    return KELP_OK;
}

kelp_error_t com_channel_peek(uint16_t channel_id, uint8_t* byte) {
    uint32_t saved_irq = channel_spin_lock(channel_id);

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_NOT_CONNECTED;
    }

    com_channel_t* channel = &com_channels[channel_id];
    channel_fifo_t* fifo;

    if (is_owner_of_channel_no_lock(channel_id)) {
        fifo = &channel->fifo_rx;
    }
    else {
        fifo = &channel->fifo_tx;
    }

    if (!fifo->full) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_CHANNEL_EMPTY;
    }

    const uint32_t fifo_count = fifo->count;

    if (fifo_count < 1) {
        channel_spin_unlock(channel_id, saved_irq);
        return KELP_CHANNEL_EMPTY;
    }

    *byte = fifo->bytes[0];

    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(channel_id, saved_irq);
    return KELP_OK;
}

void com_channel_wait_until_writable(uint16_t channel_id) {
    for (uint16_t t = 0; t < CHANNEL_BLOCKING_TIMEOUT_MS; t++) {
        if (is_channel_ready_to_write(channel_id)) {
            break;
        }

        task_sleep_ms(1);
    }
}

void com_channel_wait_until_readable(uint16_t channel_id) {
    for (uint16_t t = 0; t < CHANNEL_BLOCKING_TIMEOUT_MS; t++) {
        if (is_channel_ready_to_read(channel_id)) {
            break;
        }

        task_sleep_ms(1);
    }
}