//
// Created by wolfboy on 7/29/25.
//

#include "channel_internal.h"
#include "channel.h"

#include "scheduler.h"
#include "scheduler_internal.h"
#include "spinlock_internal.h"
#include "hardware/sync.h"

com_channel_t com_channels[NUM_CHANNELS];

uint8_t channel_garbage_collect() {
    // go through all the channels,
    // and check if they can be automatically removed

    for (uint16_t i = 0; i < NUM_CHANNELS; i++) {
        com_channel_t* channel = &com_channels[i];

        if (channel->state == CHANNEL_FREE) {
            continue;
        }

        // remove it if it's owner no longer exists
        if (!task_exists(channel->owner->id)) {
            com_channel_free(i);
            continue;
        }

        // remove it if it's set to auto free
        if (channel->can_auto_free) {
            if (channel->inactivity_cooldown > 0) {
                channel->inactivity_cooldown -= 101;
            }
            else {
                com_channel_free(i);
            }
        }
    }

    return 0;
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
    const uint32_t saved_irq = channel_spin_lock();

    bool is_owner = is_owner_of_channel_no_lock(channel_id);

    channel_spin_unlock(saved_irq);
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
    const uint32_t saved_irq = channel_spin_lock();

    bool is_connected = is_connected_to_channel_no_lock(channel_id);

    channel_spin_unlock(saved_irq);
    return is_connected;
}

int32_t get_connected_channels_no_lock(uint16_t* channel_ids, const uint16_t size) {
    uint16_t num_connected = 0;

    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        if (is_connected_to_channel_no_lock(c)) {
            if (num_connected >= size) {
                return -1;
            }

            channel_ids[num_connected] = c;
            num_connected++;
        }
    }

    return num_connected;
}

// TODO: Optimize with cacheing
int32_t get_connected_channels(uint16_t* channel_ids, const uint16_t size) {
    const uint32_t saved_irq = channel_spin_lock();

    int32_t num_connected = get_connected_channels_no_lock(channel_ids, size);

    channel_spin_unlock(saved_irq);
    return num_connected;
}

int32_t com_channel_request(uint32_t with_pid, bool autoFree) {
    const uint32_t saved_irq = channel_spin_lock();

    task_t* with = NULL;
    com_channel_t* channel = NULL;
    int32_t channel_id = -1;

    // check if `with_pid` matches the current (callee's) pid
    task_t* current_task = get_current_task();
    if (current_task->id == with_pid) {
        channel_spin_unlock(saved_irq);
        return -3;
    }

    // find task with specified pid
    scheduler_spin_lock_unsafe();
    for (uint32_t t = 0; t < num_tasks; t++) {
        if (tasks[t].id == with_pid) {
            with = &tasks[t];
            break;
        }
    }
    scheduler_spin_unlock_unsafe();

    // if we could not find the task with `with_pid` return
    if (with == NULL) {
        channel_spin_unlock(saved_irq);
        return -2;
    }

    // check if a channel already connects these two channels
    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        channel = &com_channels[c];
        if (channel->state == CHANNEL_CONNECTED) {
            if (channel->owner->id == current_task->id &&
                channel->partner->id == with_pid) {
                channel_spin_unlock(saved_irq);
                return c; // remind them they already have this one
            }
        }
    }

    // check for free channels
    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        if (com_channels[c].state == CHANNEL_FREE) {
            channel = &com_channels[c];
            channel_id = c;
            break;
        }
    }

    // if there are none, return
    if (channel == NULL) {
        channel_spin_unlock(saved_irq);
        return -1;
    }

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

    channel_spin_unlock(saved_irq);
    return channel_id;
}

int32_t com_channel_request_blocking(uint32_t with_pid, bool autoFree) {
    int32_t error = 0;
    for (uint16_t t = 0; t < CHANNEL_BLOCKING_TIMEOUT_MS; t++) {
        error = com_channel_request(with_pid, autoFree);
        if (error >= 0) {
            break;
        }
        task_sleep_ms(1);
    }

    return error;
}

int32_t com_channel_free(const uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock();

    // check if channel exists
    if (channel_id >= NUM_CHANNELS) {
        channel_spin_unlock(saved_irq);
        return -1;
    }

    // check if the current task is owner of the channel
    if (!is_owner_of_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
        return -2;
    }

    com_channel_t* channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        channel_spin_unlock(saved_irq);
        return -3;
    }

    // empty channel of contents to prevent spying
    for (uint32_t i = 0; i < CHANNEL_SIZE; ++i) {
        channel->fifo_rx.bytes[i] = 0;
        channel->fifo_tx.bytes[i] = 0;
    }

    channel->fifo_rx.count = 0;
    channel->fifo_tx.count = 0;

    channel->fifo_rx.full = 0;
    channel->fifo_tx.full = 0;

    // free channel
    channel->state = CHANNEL_FREE;
    channel->can_auto_free = false;
    channel->inactivity_cooldown = 0;

    channel_spin_unlock(saved_irq);
    return channel_id;
}

bool is_channel_ready_to_write(const uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
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
        channel_spin_unlock(saved_irq);
        return false;
    }

    channel_spin_unlock(saved_irq);
    return true;
}

int32_t com_channel_write(const uint16_t channel_id, const uint8_t* bytes, const uint16_t size) {
    const uint32_t saved_irq = channel_spin_lock();

    if (size > CHANNEL_SIZE) {
        channel_spin_unlock(saved_irq);
        return -1;
    }

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
        return -2;
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
        channel_spin_unlock(saved_irq);
        return -3;
    }

    for (int b = 0; b < size; b++) {
        fifo->bytes[b] = bytes[b];
    }

    fifo->count = size;
    fifo->full = 1;
    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(saved_irq);
    return size;
}

int32_t com_channel_write_blocking(uint16_t channel_id, const uint8_t* bytes, uint16_t size) {
    com_channel_wait_until_writable(channel_id);

    return com_channel_write(channel_id, bytes, size);
}

bool is_channel_ready_to_read(const uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
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
        channel_spin_unlock(saved_irq);
        return false;
    }

    channel_spin_unlock(saved_irq);
    return true;
}

int64_t com_channel_read(const uint16_t channel_id, uint8_t* buffer, const uint16_t size) {
    uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
        return -2;
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
        channel_spin_unlock(saved_irq);
        return -3;
    }

    uint32_t fifo_count = fifo->count;

    if (size < fifo_count) {
        channel_spin_unlock(saved_irq);
        return -1;
    }

    for (int b = 0; b < fifo_count; b++) {
        buffer[b] = fifo->bytes[b];
    }

    fifo->count = 0;
    fifo->full = 0;
    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(saved_irq);
    return fifo_count;
}

int64_t com_channel_read_blocking(uint16_t channel_id, uint8_t* buffer, uint16_t size) {
    com_channel_wait_until_readable(channel_id);

    return com_channel_read(channel_id, buffer, size);
}

int64_t com_channel_read_no_reset(const uint16_t channel_id, uint8_t* buffer, const uint16_t size) {
    uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
        return -2;
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
        channel_spin_unlock(saved_irq);
        return -3;
    }

    uint32_t fifo_count = fifo->count;

    if (size < fifo_count) {
        channel_spin_unlock(saved_irq);
        return -1;
    }

    for (int b = 0; b < fifo_count; b++) {
        buffer[b] = fifo->bytes[b];
    }

    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(saved_irq);
    return fifo_count;
}

uint8_t com_channel_peek(const uint16_t channel_id) {
    uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
        return 0;
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
        channel_spin_unlock(saved_irq);
        return 0;
    }

    const uint32_t fifo_count = fifo->count;

    if (fifo_count < 1) {
        channel_spin_unlock(saved_irq);
        return 0;
    }

    const uint8_t byte = fifo->bytes[0];

    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(saved_irq);
    return byte;
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

