//
// Created by wolfboy on 7/29/25.
//

#ifndef CHANNEL_INTERNAL_H
#define CHANNEL_INTERNAL_H

#include <stdint.h>

#include "kernel_config.h"
#include "scheduler_internal.h"
#include "channel.h"

typedef enum {
    CHANNEL_FREE,
    CHANNEL_ALLOCATED,
    CHANNEL_CONNECTED
} channel_state_t;

typedef struct {
    uint8_t bytes[CHANNEL_SIZE];
    uint8_t full;
    uint16_t count;
} channel_fifo_t;

typedef struct {
    task_t* owner;
    task_t* partner;
    uint16_t inactivity_cooldown;
    uint8_t can_auto_free;
    channel_fifo_t fifo_rx;
    channel_fifo_t fifo_tx;
    channel_state_t state;
} com_channel_t;

extern com_channel_t com_channels[NUM_CHANNELS];

/**
 * Find and free up inactive channels.
 * @return
 */
uint8_t channel_garbage_collect();

#endif //CHANNEL_INTERNAL_H
