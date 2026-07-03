//
// Created by wolfboy on 7/29/25.
// Streaming FIFO implementation optimized for throughput.
//

#include "channel_internal.h"
#include "channel.h"

#include <stdlib.h>

#include "scheduler.h"
#include "scheduler_internal.h"
#include "spinlock_internal.h"
#include "hardware/sync.h"

com_channel_t com_channels[NUM_CHANNELS];

// ============================================================
// Streaming FIFO helpers (circular buffer)
// ============================================================

/**
 * Initialize a channel FIFO to empty state.
 * capacity is set to CHANNEL_SIZE - 1 to reserve one slot
 * for the full/empty distinction.
 */
static inline void fifo_init(channel_fifo_t* fifo) {
    fifo->head = 0;
    fifo->tail = 0;
    fifo->count = 0;
    fifo->capacity = CHANNEL_SIZE - 1;
    fifo->full = 0;
}

/**
 * Write a single byte to the FIFO.
 * Returns 1 on success, 0 if full.
 * Must be called with spinlock held.
 */
static inline int fifo_write_byte(channel_fifo_t* fifo, uint8_t byte) {
    if (fifo->count >= fifo->capacity) {
        return 0; // full
    }
    fifo->bytes[fifo->head] = byte;
    fifo->head = (fifo->head + 1) % CHANNEL_SIZE;
    fifo->count++;
    fifo->full = (fifo->count >= fifo->capacity);
    return 1;
}

/**
 * Read a single byte from the FIFO.
 * Returns 1 on success, 0 if empty.
 * Sets *byte to the read value on success.
 * Must be called with spinlock held.
 */
static inline int fifo_read_byte(channel_fifo_t* fifo, uint8_t* byte) {
    if (fifo->count == 0) {
        return 0; // empty
    }
    *byte = fifo->bytes[fifo->tail];
    fifo->tail = (fifo->tail + 1) % CHANNEL_SIZE;
    fifo->count--;
    fifo->full = 0;
    return 1;
}

/**
 * Check if FIFO has space for at least `space` bytes.
 * Must be called with spinlock held.
 */
static inline int fifo_has_space(channel_fifo_t* fifo, uint16_t space) {
    return fifo->count + space <= fifo->capacity;
}

/**
 * Check if FIFO has at least `needed` bytes available.
 * Must be called with spinlock held.
 */
static inline int fifo_has_data(channel_fifo_t* fifo, uint16_t needed) {
    return fifo->count >= needed;
}

/**
 * Get number of bytes available to read.
 * Must be called with spinlock held.
 */
static inline uint16_t fifo_available(channel_fifo_t* fifo) {
    return fifo->count;
}

/**
 * Get number of bytes free for writing.
 * Must be called with spinlock held.
 */
static inline uint16_t fifo_free_space(channel_fifo_t* fifo) {
    return fifo->capacity - fifo->count;
}

/**
 * Drain FIFO contents to prevent spying.
 * Must be called with spinlock held.
 */
static inline void fifo_clear(channel_fifo_t* fifo) {
    for (uint16_t i = 0; i < CHANNEL_SIZE; i++) {
        fifo->bytes[i] = 0;
    }
    fifo->head = 0;
    fifo->tail = 0;
    fifo->count = 0;
    fifo->full = 0;
}

// ============================================================
// Channel garbage collection
// ============================================================

uint8_t channel_garbage_collect() {
    // go through all the channels,
    // and check if they can be automatically removed

    for (uint16_t i = 0; i < NUM_CHANNELS; i++) {
        com_channel_t* channel = &com_channels[i];

        if (channel->state == CHANNEL_FREE) {
            continue;
        }

        // remove it if its owner no longer exists
        if (!task_exists(channel->owner->id)) {
            com_channel_free(i);
            continue;
        }

        // remove it if its set to auto free
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

// ============================================================
// Channel initialization
// ============================================================

kelp_error_t init_channels() {
    uint32_t saved_irq = channel_spin_lock();

    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        // alloc memory for the fifos
        com_channel_t* channel = &com_channels[c];
        channel->state = CHANNEL_FREE;

        void* rx_memory = malloc(sizeof(uint8_t) * CHANNEL_SIZE);
        if (rx_memory == NULL) {
            channel_spin_unlock(saved_irq);
            return KELP_MEMORY;
        }

        void* tx_memory = malloc(sizeof(uint8_t) * CHANNEL_SIZE);
        if (tx_memory == NULL) {
            channel_spin_unlock(saved_irq);
            free(rx_memory);
            return KELP_MEMORY;
        }

        channel->fifo_rx.bytes = (uint8_t*) rx_memory;
        channel->fifo_tx.bytes = (uint8_t*) tx_memory;

        // Initialize FIFO structures
        fifo_init(&channel->fifo_rx);
        fifo_init(&channel->fifo_tx);
    }
    channel_spin_unlock(saved_irq);
    return KELP_OK;
}

// ============================================================
// Channel management helpers
// ============================================================

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

kelp_error_t get_connected_channels_no_lock(uint16_t* channel_ids, uint16_t* num_connected, uint16_t size) {
    *num_connected = 0;

    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        if (is_connected_to_channel_no_lock(c)) {
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
    const uint32_t saved_irq = channel_spin_lock();

    kelp_error_t error = get_connected_channels_no_lock(channel_ids, num_connected, size);

    channel_spin_unlock(saved_irq);
    return error;
}

uint32_t get_channel_partner_pid(uint16_t channel_id) {

    if (is_connected_to_channel(channel_id)) {
        if (is_owner_of_channel(channel_id)) {
            return com_channels[channel_id].partner->id;
        } else {
            return com_channels[channel_id].owner->id;
        }
    }

    return 0;
}

// ============================================================
// Channel request/connection
// ============================================================

kelp_error_t com_channel_request(uint32_t with_pid, bool autoFree, uint16_t* channel_id) {
    const uint32_t saved_irq = channel_spin_lock();

    task_t* with = NULL;
    com_channel_t* channel = NULL;

    // check if `with_pid` matches the current (callee's) pid
    task_t* current_task = get_current_task();
    if (current_task->id == with_pid) {
        channel_spin_unlock(saved_irq);
        return KELP_INVALID_ID;
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
        return KELP_INVALID_ID;
    }

    // check if a channel already connects these two channels
    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        channel = &com_channels[c];
        if (channel->state == CHANNEL_CONNECTED) {
            if (channel->owner->id == current_task->id &&
                channel->partner->id == with_pid) {
                channel_spin_unlock(saved_irq);
                *channel_id = c; // remind them they already have this one
                return KELP_OK;
            }
        }
    }

    // check for free channels
    for (uint16_t c = 0; c < NUM_CHANNELS; c++) {
        if (com_channels[c].state == CHANNEL_FREE) {
            channel = &com_channels[c];
            *channel_id = c;
            break;
        }
    }

    // if there are none, return
    if (channel == NULL) {
        channel_spin_unlock(saved_irq);
        return KELP_NONE_FREE;
    }

    channel->state = CHANNEL_ALLOCATED;

    channel->owner = current_task;
    channel->partner = with;

    // Initialize FIFOs for streaming
    fifo_init(&channel->fifo_rx);
    fifo_init(&channel->fifo_tx);

    channel->state = CHANNEL_CONNECTED;
    channel->can_auto_free = autoFree;
    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(saved_irq);
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

// ============================================================
// Channel free
// ============================================================

kelp_error_t com_channel_free(uint16_t channel_id) {
    const uint32_t saved_irq = channel_spin_lock();

    // check if channel exists
    if (channel_id >= NUM_CHANNELS) {
        channel_spin_unlock(saved_irq);
        return KELP_INVALID_ID;
    }

    // check if the current task is owner of the channel
    if (!is_owner_of_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
        return KELP_NOT_OWNER;
    }

    com_channel_t* channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        channel_spin_unlock(saved_irq);
        return KELP_UNALLOCATED;
    }

    // clear FIFOs to prevent spying
    fifo_clear(&channel->fifo_rx);
    fifo_clear(&channel->fifo_tx);

    // free channel
    channel->state = CHANNEL_FREE;
    channel->can_auto_free = false;
    channel->inactivity_cooldown = 0;

    channel_spin_unlock(saved_irq);
    return KELP_OK;
}

// ============================================================
// Streaming write with partial buffer support
// ============================================================

/**
 * Write bytes to a channel's FIFO using streaming (partial) model.
 * Writes as many bytes as will fit (up to `size`).
 * Sets *written to the number of bytes actually written.
 * Returns KELP_CHANNEL_FULL if no space available.
 * Returns KELP_NOT_CONNECTED if channel not connected.
 */
static kelp_error_t com_channel_write_streaming(uint16_t channel_id, const uint8_t* bytes, uint16_t size, uint16_t* written, bool reset_inactivity) {
    uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
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

    uint16_t free = fifo_free_space(fifo);
    if (free == 0) {
        channel_spin_unlock(saved_irq);
        return KELP_CHANNEL_FULL;
    }

    // Write as many bytes as will fit
    uint16_t to_write = (size < free) ? size : free;
    for (uint16_t b = 0; b < to_write; b++) {
        fifo_write_byte(fifo, bytes[b]);
    }

    if (written) {
        *written = to_write;
    }

    if (reset_inactivity) {
        channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;
    }

    channel_spin_unlock(saved_irq);
    return KELP_OK;
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

    if (fifo_free_space(fifo) == 0) {
        channel_spin_unlock(saved_irq);
        return false;
    }

    channel_spin_unlock(saved_irq);
    return true;
}

kelp_error_t com_channel_write(uint16_t channel_id, const uint8_t* bytes, uint16_t size, uint16_t* written) {
    return com_channel_write_streaming(channel_id, bytes, size, written, true);
}

kelp_error_t com_channel_write_blocking(uint16_t channel_id, const uint8_t* bytes, uint16_t size, uint16_t* written) {
    com_channel_wait_until_writable(channel_id);

    return com_channel_write(channel_id, bytes, size, written);
}

// ============================================================
// Streaming read with partial buffer support
// ============================================================

/**
 * Read bytes from a channel's FIFO using streaming (partial) model.
 * Reads up to `size` bytes (or all available if less).
 * Sets *read to the number of bytes actually read.
 * Returns KELP_CHANNEL_EMPTY if no data available.
 * Returns KELP_NOT_CONNECTED if channel not connected.
 */
static kelp_error_t com_channel_read_streaming(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size, bool reset_inactivity) {
    uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
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

    uint16_t available = fifo_available(fifo);
    if (available == 0) {
        channel_spin_unlock(saved_irq);
        return KELP_CHANNEL_EMPTY;
    }

    // Read up to `size` bytes or all available (whichever is smaller)
    uint16_t to_read = (size < available) ? size : available;
    for (uint16_t b = 0; b < to_read; b++) {
        uint8_t byte;
        fifo_read_byte(fifo, &byte);
        buffer[b] = byte;
    }

    *read = to_read;

    if (reset_inactivity) {
        channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;
    }

    channel_spin_unlock(saved_irq);
    return KELP_OK;
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

    if (fifo_available(fifo) == 0) {
        channel_spin_unlock(saved_irq);
        return false;
    }

    channel_spin_unlock(saved_irq);
    return true;
}

kelp_error_t com_channel_read(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size) {
    return com_channel_read_streaming(channel_id, buffer, read, size, true);
}

kelp_error_t com_channel_read_blocking(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size) {
    com_channel_wait_until_readable(channel_id);

    return com_channel_read(channel_id, buffer, read, size);
}

kelp_error_t com_channel_read_no_reset(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size) {
    return com_channel_read_streaming(channel_id, buffer, read, size, false);
}

kelp_error_t com_channel_peek(uint16_t channel_id, uint8_t* byte) {
    uint32_t saved_irq = channel_spin_lock();

    if (!is_connected_to_channel_no_lock(channel_id)) {
        channel_spin_unlock(saved_irq);
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

    if (fifo_available(fifo) == 0) {
        channel_spin_unlock(saved_irq);
        return KELP_CHANNEL_EMPTY;
    }

    // Peek at the first available byte without removing it
    uint16_t tail = fifo->tail;
    *byte = fifo->bytes[tail];

    channel->inactivity_cooldown = CHANNEL_AUTO_FREE_DELAY;

    channel_spin_unlock(saved_irq);
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
