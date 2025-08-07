//
// Created by wolfboy on 7/29/25.
//

#include "channel_internal.h"
#include "channel.h"
#include "hardware/sync.h"

com_channel_t com_channels[NUM_CHANNELS];

// TODO Should use channel spinlock when they are implemented
//   vvvv
bool is_owner_of_channel(uint16_t channel_id) {
    uint32_t saved_irq = save_and_disable_interrupts();

    if (channel_id >= NUM_CHANNELS) {
        restore_interrupts(saved_irq);
        return false;
    }

    com_channel_t *channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        restore_interrupts(saved_irq);
        return false;
    }

    // TODO Should use scheduler spinlock when they are implemented
    //   vvvv
    bool is_owner = get_current_task() == channel->owner;

    restore_interrupts(saved_irq);
    return is_owner;
}

// TODO Should use channel spinlock when they are implemented
//   vvvv
bool is_connected_to_channel(uint16_t channel_id) {
    uint32_t saved_irq = save_and_disable_interrupts();

    if (channel_id >= NUM_CHANNELS) {
        restore_interrupts(saved_irq);
        return false;
    }

    com_channel_t *channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        restore_interrupts(saved_irq);
        return false;
    }

    // TODO Should use scheduler spinlock when they are implemented
    //   vvvv
    task_t *current_task = get_current_task();
    bool is_connected = (current_task == channel->owner | current_task == channel->partner);

    restore_interrupts(saved_irq);
    return is_connected;
}

uint32_t get_connected_channels(uint16_t *channel_ids, uint16_t size) {
    uint32_t saved_irq = save_and_disable_interrupts();
    uint32_t num_connected = 0;

    for (uint32_t c = 0; c < NUM_CHANNELS; c++) {
        if (is_connected_to_channel(c)) {

            if (num_connected >= size) {
                restore_interrupts(saved_irq);
                return -1;
            }

            channel_ids[num_connected] = c;
            num_connected++;
        }
    }

    restore_interrupts(saved_irq);
    return num_connected;
}

// TODO Should use channel spinlock when they are implemented
//   vvvv
int32_t com_channel_request(uint32_t with_pid) {
    uint32_t saved_irq = save_and_disable_interrupts();

    task_t *with = NULL;
    com_channel_t *channel = NULL;
    int32_t channel_id = -1;

    // check if `with_pid` matches the current (callee's) pid
    // TODO Should use scheduler spinlock when they are implemented
    //   vvvv
    task_t *current_task = get_current_task();
    if (current_task->id == with_pid) {
        restore_interrupts(saved_irq);
        return -3;
    }

    // find task with specified pid
    // TODO Should use scheduler spinlock when they are implemented
    //   vvvv
    for (uint32_t t = 0; t < num_tasks; t++) {
        if (tasks[t].id == with_pid) {
            with = &tasks[t];
            break;
        }
    }

    // if we could not find the task with `with_pid` return
    if (with == NULL) {
        restore_interrupts(saved_irq);
        return -2;
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
        restore_interrupts(saved_irq);
        return  -1;
    }

    channel->state = CHANNEL_ALLOCATED;

    // TODO Should use scheduler spinlock when they are implemented
    //   vvvv
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

    restore_interrupts(saved_irq);
    return channel_id;
}

int32_t com_channel_free(uint16_t channel_id) {
    uint32_t saved_irq = save_and_disable_interrupts();

    // check if channel exists
    if (channel_id >= NUM_CHANNELS) {
        restore_interrupts(saved_irq);
        return -1;
    }

    // check if the current task is owner of the channel
    if (!is_owner_of_channel(channel_id)) {
        restore_interrupts(saved_irq);
        return -2;
    }

    // TODO Should use channel spinlock when they are implemented
    //   vvvv
    com_channel_t *channel = &com_channels[channel_id];

    if (channel->state == CHANNEL_FREE) {
        restore_interrupts(saved_irq);
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

    restore_interrupts(saved_irq);
    return channel_id;
}

bool channel_ready_to_write(uint16_t channel_id) {
    uint32_t saved_irq = save_and_disable_interrupts();

    if (!is_connected_to_channel(channel_id)) {
        restore_interrupts(saved_irq);
        return false;
    }

    com_channel_t *channel = &com_channels[channel_id];
    channel_fifo_t *fifo;

    if (is_owner_of_channel(channel_id)) {
        fifo = &channel->fifo_tx;
    } else {
        fifo = &channel->fifo_rx;
    }

    // TODO Should use channel spinlock when they are implemented
    //   vvvv
    if (fifo->full) {
        restore_interrupts(saved_irq);
        return false;
    }

    restore_interrupts(saved_irq);
    return true;
}

int32_t com_channel_write(uint16_t channel_id, const uint8_t *bytes, uint32_t size) {
    uint32_t saved_irq = save_and_disable_interrupts();

    if (size > CHANNEL_SIZE) {
        restore_interrupts(saved_irq);
        return -1;
    }

    if (!is_connected_to_channel(channel_id)) {
        restore_interrupts(saved_irq);
        return -2;
    }

    com_channel_t *channel = &com_channels[channel_id];
    channel_fifo_t *fifo;

    if (is_owner_of_channel(channel_id)) {
        fifo = &channel->fifo_tx;
    } else {
        fifo = &channel->fifo_rx;
    }

    // TODO Should use channel spinlock when they are implemented
    //   vvvv
    for (int b = 0; b < size; b++) {
        fifo->bytes[b] = bytes[b];
    }

    fifo->count = size;
    fifo->full = 1;

    restore_interrupts(saved_irq);
    return size;
}

bool channel_ready_to_read(uint16_t channel_id) {
    uint32_t saved_irq = save_and_disable_interrupts();

    if (!is_connected_to_channel(channel_id)) {
        restore_interrupts(saved_irq);
        return false;
    }

    com_channel_t *channel = &com_channels[channel_id];
    channel_fifo_t *fifo;

    if (is_owner_of_channel(channel_id)) {
        fifo = &channel->fifo_rx;
    } else {
        fifo = &channel->fifo_tx;
    }

    // TODO Should use channel spinlock when they are implemented
    //   vvvv
    if (!fifo->full) {
        restore_interrupts(saved_irq);
        return false;
    }

    restore_interrupts(saved_irq);
    return true;
}

int32_t com_channel_read(uint16_t channel_id, uint8_t *buffer, uint32_t size) {
    uint32_t saved_irq = save_and_disable_interrupts();

    if (!is_connected_to_channel(channel_id)) {
        restore_interrupts(saved_irq);
        return -2;
    }

    com_channel_t *channel = &com_channels[channel_id];
    channel_fifo_t *fifo;

    if (is_owner_of_channel(channel_id)) {
        fifo = &channel->fifo_rx;
    } else {
        fifo = &channel->fifo_tx;
    }

    // TODO Should use channel spinlock when they are implemented
    //   vvvv
    uint32_t fifo_count = fifo->count;

    if (size < fifo_count) {
        restore_interrupts(saved_irq);
        return -1;
    }

    for (int b = 0; b < fifo_count; b++) {
        buffer[b] = fifo->bytes[b];
    }

    fifo->count = 0;
    fifo->full = 0;

    restore_interrupts(saved_irq);
    return fifo_count;
}


