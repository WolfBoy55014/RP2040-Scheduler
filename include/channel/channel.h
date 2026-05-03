//
// Created by wolfboy on 8/7/25.
//

#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stdint.h>

#include "error_codes.h"

#define CHANNEL_BLOCKING_TIMEOUT_MS 100

/**
 * @brief Check if the current task owns a channel or not
 * @param channel_id ID of the channel
 * @return Either or not the current task owns this channel
 */
bool is_owner_of_channel(uint16_t channel_id);

/**
 * @brief Check if current task is connected to a channel or not
 * @param channel_id ID of the channel
 * @return either or not the task is connected to a channel
 */
bool is_connected_to_channel(uint16_t channel_id);

/**
 * @brief Get which channels are connected to the current task
 * @param channel_ids Array to populate with connected channel ids
 * @param num_connected Pointer to save the number of connected channels to
 * @param size Length of @code channel_ids@endcode array
 * @return An error code
 */
kelp_error_t get_connected_channels(uint16_t* channel_ids, uint16_t* num_connected, uint16_t size);

/**
 * @brief Request to be connected to the task with pid @code with_pid@endcode.
 * If there is a free communication channel,
 * a channel will be allocated and connected to the current task and task @code with_pid@endcode.
 * The current task will become the owner of the channel.
 * If @code autoFree@endcode is true, the channel will be automatically deallocated after inactivity
 *
 * @param with_pid pid of the task the current task will be connected to
 * @param autoFree either or not the channel should be automatically freed on inactivity
 * @param channel_id Pointer to save the channel ID to
 * @return An error code
 */
kelp_error_t com_channel_request(uint32_t with_pid, bool autoFree, uint16_t* channel_id);

/**
 * @brief Request to be connected to the task with pid @code with_pid@endcode, blocking until a channel is free
 * If there is a free communication channel,
 * a channel will be allocated and connected to the current task and task @code with_pid@endcode.
 * The current task will become the owner of the channel.
 * If @code autoFree@endcode is true, the channel will be automatically deallocated after inactivity
 *
 * @param with_pid pid of the task the current task will be connected to
 * @param autoFree either or not the channel should be automatically freed on inactivity
 * @param channel_id Pointer to save the channel ID to
 * @return An error code
 */
kelp_error_t com_channel_request_blocking(uint32_t with_pid, bool autoFree, uint16_t* channel_id);

/**
 * @brief Disconnect and free a communication channel
 * @param channel_id ID if the channel to free
 * @return An error code
 */
kelp_error_t com_channel_free(uint16_t channel_id);

/**
 * @brief Check if channel is empty and ready to write to
 * @param channel_id ID of channel
 * @return Either or not the channel is empty and ready to write to
 */
bool is_channel_ready_to_write(uint16_t channel_id);

/**
 * @brief Write a buffer of data to a channel
 * @param channel_id ID of the channel to write to
 * @param bytes Array of data to write
 * @param size The length of @code bytes@endcode
 * @return An error code
 */
kelp_error_t com_channel_write(uint16_t channel_id, const uint8_t* bytes, uint16_t size);

/**
 * @brief Write a buffer of data to a channel, but will block until data can be written
 * @param channel_id ID of the channel to write to
 * @param bytes Array of data to write
 * @param size The length of @code bytes@endcode
 * @return An error code
 */
kelp_error_t com_channel_write_blocking(uint16_t channel_id, const uint8_t* bytes, uint16_t size);

/**
 * @brief Check if channel has data ready to read
 * @param channel_id ID of channel
 * @return Either or not the channel full of new data
 */
bool is_channel_ready_to_read(uint16_t channel_id);

/**
 * @brief Read the data from a channel and copy it to a provided buffer
 * @param channel_id ID of the channel to read from
 * @param buffer Array to copy data to
 * @param read A pointer to save the amount of data read to
 * @param size Size of the provided buffer
 * @return An error code
 */
kelp_error_t com_channel_read(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size);

/**
 * @brief Read the data from a channel and copy it to a provided buffer, but will block until data can be read
 * @param channel_id ID of the channel to read from
 * @param buffer Array to copy data to
 * @param read A pointer to save the amount of data read to
 * @param size Size of the provided buffer
 * @return An error code
 */
kelp_error_t com_channel_read_blocking(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size);

/**
 * @brief Read the data from a channel and copy it to a provided buffer without clearing the channel after
 * @param channel_id ID of the channel to read from
 * @param buffer Array to copy data to
 * @param read A pointer to save the amount of data read to
 * @param size Size of the provided buffer
 * @return An error code
 */
kelp_error_t com_channel_read_no_reset(uint16_t channel_id, uint8_t* buffer, uint16_t* read, uint16_t size);

/**
 * Preview the first byte of a channel, without resetting it \n
 * Useful for checking for protocols
 * @param channel_id ID of the channel to peek into
 * @param byte A pointer to save the byte to
 * @return An error code
 */
kelp_error_t com_channel_peek(uint16_t channel_id, uint8_t* byte);

void com_channel_wait_until_writable(uint16_t channel_id);

void com_channel_wait_until_readable(uint16_t channel_id);

#endif //CHANNEL_H
