//
// channel_speed_test.c
// Dedicated benchmark for channel throughput
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "channel.h"
#include "scheduler.h"
#include "kernel_config.h"

#define TEST_DATA_SIZE 4096
#define BUFFER_SIZE (7 + TEST_DATA_SIZE)  // 7-byte header + data

static uint8_t tx_buffer[BUFFER_SIZE];
static uint8_t rx_buffer[BUFFER_SIZE];

// Sender task
void channel_sender_task(void *args) {
    uint16_t channel_id = *(uint16_t*)args;
    uint32_t total_sent = 0;
    uint32_t start_us = time_us_32();

    for (uint32_t i = 0; i < 1000; i++) {
        // Fill 7-byte header: type(1) + reason(2) + size(4)
        tx_buffer[0] = 0x08; // COM_TYPE_STR_I
        tx_buffer[1] = 0x00; // reason high
        tx_buffer[2] = 0x00; // reason low
        tx_buffer[3] = TEST_DATA_SIZE >> 24;
        tx_buffer[4] = (TEST_DATA_SIZE >> 16) & 0xFF;
        tx_buffer[5] = (TEST_DATA_SIZE >> 8) & 0xFF;
        tx_buffer[6] = TEST_DATA_SIZE & 0xFF;

        // Fill data
        for (uint32_t b = 0; b < TEST_DATA_SIZE; b++) {
            tx_buffer[b + 7] = (uint8_t)(b & 0xFF);
        }

        // Write (uses memcpy internally)
        kelp_error_t result = com_channel_write(channel_id, tx_buffer, BUFFER_SIZE);
        if (result < 0) {
            printf("Sender error at iteration %lu: %d\n", i, result);
            break;
        }
        total_sent += result;
    }

    uint32_t end_us = time_us_32();
    uint32_t duration_us = end_us - start_us;
    if (duration_us > 0) {
        float throughput = (float)total_sent / (duration_us / 1000000.0f);
        printf("Sender: %lu bytes in %lu us (%.2f KB/s, %.2f MB/s)\n",
               total_sent, duration_us,
               throughput / 1024.0f,
               throughput / 1048576.0f);
    }
}

// Receiver task
void channel_receiver_task(void *args) {
    uint16_t channel_id = *(uint16_t*)args;
    uint32_t total_recv = 0;
    uint32_t start_us = time_us_32();

    for (uint32_t i = 0; i < 1000; i++) {
        uint16_t bytes_read = 0;
        kelp_error_t result = com_channel_read(channel_id, rx_buffer, &bytes_read, BUFFER_SIZE);
        if (result < 0) {
            printf("Receiver error at iteration %lu: %d\n", i, result);
            break;
        }
        total_recv += bytes_read;
    }

    uint32_t end_us = time_us_32();
    uint32_t duration_us = end_us - start_us;
    if (duration_us > 0) {
        float throughput = (float)total_recv / (duration_us / 1000000.0f);
        printf("Receiver: %lu bytes in %lu us (%.2f KB/s, %.2f MB/s)\n",
               total_recv, duration_us,
               throughput / 1024.0f,
               throughput / 1048576.0f);
    }
}

// Create the test
void init_channel_speed_test(void) {
    uint16_t channel_id;
    kelp_error_t ret = com_channel_request(2, false, &channel_id);
    if (ret >= 0) {
        uint16_t *ch_id_ptr = (uint16_t*)malloc(sizeof(uint16_t));
        *ch_id_ptr = channel_id;

        printf("Channel speed test on channel %u\n", channel_id);
        printf("Sending 1000 x 4096 byte messages\n\n");

        task_create(channel_sender_task, ch_id_ptr, 512, 1);
        task_create(channel_receiver_task, ch_id_ptr, 512, 2);
    } else {
        printf("Failed to create channel: %d\n", ret);
    }
}
