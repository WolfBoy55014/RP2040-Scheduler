//
// Created by wolfboy on 8/7/25.
//

#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stdint.h>

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
 * @param size Length of @code channel_ids@endcode array
 * @return -1 if @code channel_ids@endcode was not long enough, the number of connected channels if positive
 */
uint32_t get_connected_channels(uint16_t *channel_ids, uint16_t size);

/**
 * @brief Request to be connected to the task with pid @code with_pid@endcode.
 * If there is a free communication channel,
 * a channel will be allocated and connected to the current task and task @code with_pid@endcode.
 * The current task will become the owner of the channel.
 *
 * @param with_pid pid of the task the current task will be connected to
 * @return -1 if no available channels,\n -2 if no tasks have pid @code with_pid@endcode,\n -3 if the current task has pid @code with_pid@endcode,\n 0+ channel id if successful.
 */
int32_t com_channel_request(uint32_t with_pid);

/**
 * @brief Disconnect and free a communication channel
 * @param channel_id ID if the channel to free
 * @return -1 if the channel does not exist,\n -2 if the current task is not the owner of the channel,\n -3 if the channel is not allocated.
 */
int32_t com_channel_free(uint16_t channel_id);

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
 * @return -1 if @code bytes@endcode is too long,\n -2 if the current task is not connected to the channel,\n the number of bytes written if successful
 */
int32_t com_channel_write(uint16_t channel_id, const uint8_t *bytes, uint16_t size);

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
 * @param size Size of the provided buffer
 * @return -1 if @code buffer@endcode is too short to store the data,\n -2 if the current task is not connected to the channel,\n the amount of data read if successful
 */
int32_t com_channel_read(uint16_t channel_id, uint8_t *buffer, uint16_t size);

#endif //CHANNEL_H
