//
// Created by wolfboy on 11/27/2025.
//

#include "com_channel_protocol.h"

#include <string.h>

#include "scheduler.h"
#include "channel_internal.h"
#include  "error_codes.h"

kelp_error_t com_send_uint32(const uint16_t channel_id, const uint32_t data, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    uint8_t bytes[7];

    bytes[0] = COM_TYPE_UINT32;
    bytes[1] = reason >> 8;
    bytes[2] = reason;
    bytes[3] = data >> 24;
    bytes[4] = data >> 16;
    bytes[5] = data >> 8;
    bytes[6] = data;

    kelp_error_t error = com_channel_write(channel_id, bytes, 7, NULL);
    return error;
}

kelp_error_t com_get_uint32(const uint16_t channel_id, uint32_t* data, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[7];
    uint16_t bytes_read = 0;

    kelp_error_t error = com_channel_read(channel_id, bytes, &bytes_read, 7);
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_UINT32) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    if (bytes_read < 7) {
        return KELP_PROTOCOL;
    }

    uint32_t data_uint32 = bytes[3] << 24 | bytes[4] << 16 | bytes[5] << 8 | bytes[6];
    *data = data_uint32;

    *reason = bytes[1] << 8 | bytes[2];

    return KELP_OK;
}

kelp_error_t com_send_int32(const uint16_t channel_id, const int32_t data, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    uint8_t bytes[7];

    bytes[0] = COM_TYPE_INT32;
    bytes[1] = reason >> 8;
    bytes[2] = reason;
    bytes[3] = data >> 24;
    bytes[4] = data >> 16;
    bytes[5] = data >> 8;
    bytes[6] = data;

    kelp_error_t error = com_channel_write(channel_id, bytes, 7, NULL);
    return error;
}

kelp_error_t com_get_int32(const uint16_t channel_id, int32_t* data, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[7];
    uint16_t bytes_read = 0;

    kelp_error_t error = com_channel_read(channel_id, bytes, &bytes_read, 7);
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_INT32) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    if (bytes_read < 7) {
        return KELP_PROTOCOL;
    }

    int32_t data_int32 = bytes[3] << 24 | bytes[4] << 16 | bytes[5] << 8 | bytes[6];
    *data = data_int32;

    *reason = bytes[1] << 8 | bytes[2];

    return KELP_OK;
}

kelp_error_t com_send_uint64(const uint16_t channel_id, const uint64_t data, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    uint8_t bytes[11];

    bytes[0] = COM_TYPE_UINT64;
    bytes[1] = reason >> 8;
    bytes[2] = reason;
    bytes[3] = data >> 56;
    bytes[4] = data >> 48;
    bytes[5] = data >> 40;
    bytes[6] = data >> 32;
    bytes[7] = data >> 24;
    bytes[8] = data >> 16;
    bytes[9] = data >> 8;
    bytes[10] = data;

    kelp_error_t error = com_channel_write(channel_id, bytes, 11, NULL);
    return error;
}

kelp_error_t com_get_uint64(const uint16_t channel_id, uint64_t* data, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[11];
    uint16_t bytes_read = 0;

    kelp_error_t error = com_channel_read(channel_id, bytes, &bytes_read, 11);
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_UINT64) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    if (bytes_read < 11) {
        return KELP_PROTOCOL;
    }

    uint64_t data_uint64 =
        (uint64_t)bytes[3] << 56 |
        (uint64_t)bytes[4] << 48 |
        (uint64_t)bytes[5] << 40 |
        (uint64_t)bytes[6] << 32 |
        (uint64_t)bytes[7] << 24 |
        (uint64_t)bytes[8] << 16 |
        (uint64_t)bytes[9] << 8 |
        (uint64_t)bytes[10];

    *data = data_uint64;

    *reason = bytes[1] << 8 | bytes[2];

    return KELP_OK;
}

kelp_error_t com_send_int64(const uint16_t channel_id, const int64_t data, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    uint8_t bytes[11];

    bytes[0] = COM_TYPE_INT64;
    bytes[1] = reason >> 8;
    bytes[2] = reason;
    bytes[3] = data >> 56;
    bytes[4] = data >> 48;
    bytes[5] = data >> 40;
    bytes[6] = data >> 32;
    bytes[7] = data >> 24;
    bytes[8] = data >> 16;
    bytes[9] = data >> 8;
    bytes[10] = data;

    kelp_error_t error = com_channel_write(channel_id, bytes, 11, NULL);
    return error;
}

kelp_error_t com_get_int64(const uint16_t channel_id, int64_t* data, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[11];
    uint16_t bytes_read = 0;

    kelp_error_t error = com_channel_read(channel_id, bytes, &bytes_read, 11);
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_INT64) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    if (bytes_read < 11) {
        return KELP_PROTOCOL;
    }

    int64_t data_int64 =
        (uint64_t)bytes[3] << 56 |
        (uint64_t)bytes[4] << 48 |
        (uint64_t)bytes[5] << 40 |
        (uint64_t)bytes[6] << 32 |
        (uint64_t)bytes[7] << 24 |
        (uint64_t)bytes[8] << 16 |
        (uint64_t)bytes[9] << 8 |
        (uint64_t)bytes[10];

    *data = data_int64;

    *reason = bytes[1] << 8 | bytes[2];

    return KELP_OK;
}

kelp_error_t com_send_float(const uint16_t channel_id, const float data, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    union {
        float f;
        uint8_t b[4]; // shares the same memory space as f
    } d;

    d.f = data;

    uint8_t bytes[7];

    bytes[0] = COM_TYPE_FLO;
    bytes[1] = reason >> 8;
    bytes[2] = reason;
    bytes[3] = d.b[0];
    bytes[4] = d.b[1];
    bytes[5] = d.b[2];
    bytes[6] = d.b[3];

    kelp_error_t error = com_channel_write(channel_id, bytes, 7, NULL);
    return error;
}

kelp_error_t com_get_float(const uint16_t channel_id, float* data, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[7];
    uint16_t bytes_read = 0;

    kelp_error_t error = com_channel_read(channel_id, bytes, &bytes_read, 7);
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_FLO) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    if (bytes_read < 7) {
        return KELP_PROTOCOL;
    }

    memcpy(data, bytes + 3, 4);
    // WARNING: dependent on endianness of system

    *reason = bytes[1] << 8 | bytes[2];

    return KELP_OK;
}

kelp_error_t com_send_double(const uint16_t channel_id, const double data, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    union {
        double f;
        uint8_t b[8]; // shares the same memory space as f
    } d;

    d.f = data;

    uint8_t bytes[11];

    bytes[0] = COM_TYPE_DUB;
    bytes[1] = reason >> 8;
    bytes[2] = reason;
    bytes[3] = d.b[0];
    bytes[4] = d.b[1];
    bytes[5] = d.b[2];
    bytes[6] = d.b[3];
    bytes[7] = d.b[4];
    bytes[8] = d.b[5];
    bytes[9] = d.b[6];
    bytes[10] = d.b[7];

    kelp_error_t error = com_channel_write(channel_id, bytes, 11, NULL);
    return error;
}

kelp_error_t com_get_double(const uint16_t channel_id, double* data, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[11];
    uint16_t bytes_read = 0;

    int32_t error = com_channel_read(channel_id, bytes, &bytes_read, 11);
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_DUB) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    if (bytes_read < 11) {
        return KELP_PROTOCOL;
    }

    memcpy(data, bytes + 3, 8);
    // WARNING: dependent on endianness of system

    *reason = bytes[1] << 8 | bytes[2];

    return KELP_OK;
}

kelp_error_t com_send_char(const uint16_t channel_id, const char data, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    uint8_t bytes[4];

    bytes[0] = COM_TYPE_CHAR;
    bytes[1] = reason >> 8;
    bytes[2] = reason;
    bytes[3] = data;

    kelp_error_t error = com_channel_write(channel_id, bytes, 4, NULL);
    return error;
}

kelp_error_t com_get_char(const uint16_t channel_id, char* data, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_FULL; // channel empty
    }

    uint8_t bytes[4];
    uint16_t bytes_read = 0;

    int32_t error = com_channel_read(channel_id, bytes, &bytes_read, 4);
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_CHAR) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    if (bytes_read < 4) {
        return KELP_PROTOCOL;
    }

    char data_char = bytes[3];
    *data = data_char;

    *reason = bytes[1] << 8 | bytes[2];

    return KELP_OK;
}

kelp_error_t com_send_char_array(const uint16_t channel_id, const char data[], uint32_t size, const uint16_t reason) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    // streaming header: | Type (1B) | Reason (2B) | Size (4B) |
    // Total: 7 bytes. Channel handles chunking transparently.
    uint8_t header[7];
    header[0] = COM_TYPE_STR_I;
    header[1] = reason >> 8;
    header[2] = reason;
    header[3] = size >> 24;
    header[4] = size >> 16;
    header[5] = size >> 8;
    header[6] = size;

    // send header (writes whatever fits)
    uint16_t header_written = 0;
    kelp_error_t error = com_channel_write(channel_id, header, 7, &header_written);
    if (error != KELP_OK) {
        return error;
    }

    // send data (channel chunks transparently)
    uint16_t data_written = 0;
    error = com_channel_write(channel_id, (const uint8_t*)data, size, &data_written);
    if (error != KELP_OK) {
        return error;
    }

    return KELP_OK;
}

kelp_error_t com_get_char_array(uint16_t channel_id, char* data, uint32_t max_size, uint32_t* size, uint16_t* reason) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    // streaming header: | Type (1B) | Reason (2B) | Size (4B) |
    // Total: 7 bytes. Channel handles chunking transparently.
    uint8_t header[7];
    uint16_t header_read = 0;

    kelp_error_t error = com_channel_read(channel_id, header, &header_read, 7);
    if (error < 0) {
        return error;
    }

    if (header_read < 7) {
        return KELP_PROTOCOL; // incomplete header
    }

    if (header[0] != COM_TYPE_STR_I) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    *reason = header[1] << 8 | header[2];
    *size = header[3] << 24 | header[4] << 16 | header[5] << 8 | header[6];

    // receiver knows exactly how many bytes to read
    if (*size > max_size) {
        return KELP_TOO_BIG;
    }

    if (*size == 0) {
        return KELP_OK;
    }

    // accumulate data in chunks until we have it all
    uint32_t remaining = *size;
    uint32_t offset = 0;

    while (remaining > 0) {
        uint16_t chunk = (remaining > 128) ? 128 : (uint16_t)remaining;
        uint16_t chunk_read = 0;

        error = com_channel_read(channel_id, (uint8_t*)data + offset, &chunk_read, chunk);
        if (error < 0) {
            return error;
        }

        if (chunk_read == 0) {
            // no data available, wait for it
            com_channel_wait_until_readable(channel_id);
            continue;
        }

        remaining -= chunk_read;
        offset += chunk_read;
    }

    return KELP_OK;
}

kelp_error_t com_send_char_array_fast(const uint16_t channel_id, const char* data, const uint16_t size, const uint16_t reason) {

    const uint32_t packet_size = size + 3;

    if (packet_size > CHANNEL_SIZE) {
        return KELP_TOO_BIG; // array too big, increase channel size, or decrease array size
    }

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    uint8_t bytes[packet_size];

    bytes[0] = COM_TYPE_ARRAY;
    bytes[1] = reason >> 8;
    bytes[2] = reason;

    for (uint16_t i = 0; i < size; i++) {
        bytes[3 + i] = data[i];
    }

    kelp_error_t error = com_channel_write(channel_id, bytes, packet_size, NULL);
    return error;
}

kelp_error_t com_get_char_array_fast(const uint16_t channel_id, char (*data)[CHANNEL_SIZE], uint16_t* size, uint16_t* reason) {
    // TODO: How the HECK I'm I SUPPOSED to pass a pointer to an ARRAY! >:(

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[CHANNEL_SIZE];

    kelp_error_t error = com_channel_read(channel_id, bytes, size, CHANNEL_SIZE);
    *size -= 3;
    if (error != KELP_OK) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_ARRAY) {
        return KELP_WRONG_TYPE; // wrong data type
    }

    *reason = bytes[1] << 8 | bytes[2];

    for (uint16_t i = 0; i < *size; i++) {
        (*data)[i] = bytes[3 + i];
    }

    return KELP_OK;
}

kelp_error_t com_send_request(const uint16_t channel_id, const uint16_t request) {

    if (!is_channel_ready_to_write(channel_id)) {
        return KELP_CHANNEL_FULL; // current contents have not been read
    }

    uint8_t bytes[3];

    bytes[0] = COM_TYPE_REQ;
    bytes[1] = request >> 8;
    bytes[2] = request;

    kelp_error_t error = com_channel_write(channel_id, bytes, 3, NULL);
    return error;
}

kelp_error_t com_get_request(const uint16_t channel_id, uint16_t* request) {

    if (!is_channel_ready_to_read(channel_id)) {
        return KELP_CHANNEL_EMPTY; // channel empty
    }

    uint8_t bytes[3];
    uint16_t bytes_read = 0;

    int32_t error = com_channel_read(channel_id, bytes, &bytes_read, 3);
    if (error < 0) {
        // there was an error
        return error;
    }

    if (bytes[0] != COM_TYPE_REQ) {
        return -4; // wrong data type
    }

    *request = bytes[1] << 8 | bytes[2];

    return 0;
}

kelp_error_t com_send_uint32_blocking(uint16_t channel_id, uint32_t data, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_uint32(channel_id, data, reason);
}

kelp_error_t com_get_uint32_blocking(uint16_t channel_id, uint32_t* data, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_uint32(channel_id, data, reason);
}

kelp_error_t com_send_int32_blocking(uint16_t channel_id, int32_t data, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_int32(channel_id, data, reason);
}

kelp_error_t com_get_int32_blocking(uint16_t channel_id, int32_t* data, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_int32(channel_id, data, reason);
}

kelp_error_t com_send_uint64_blocking(uint16_t channel_id, uint64_t data, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_uint64(channel_id, data, reason);
}

kelp_error_t com_get_uint64_blocking(uint16_t channel_id, uint64_t* data, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_uint64(channel_id, data, reason);
}

kelp_error_t com_send_int64_blocking(uint16_t channel_id, int64_t data, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_int64(channel_id, data, reason);
}

kelp_error_t com_get_int64_blocking(uint16_t channel_id, int64_t* data, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_int64(channel_id, data, reason);
}

kelp_error_t com_send_float_blocking(uint16_t channel_id, float data, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_float(channel_id, data, reason);
}

kelp_error_t com_get_float_blocking(uint16_t channel_id, float* data, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_float(channel_id, data, reason);
}

kelp_error_t com_send_double_blocking(uint16_t channel_id, double data, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_double(channel_id, data, reason);
}

kelp_error_t com_get_double_blocking(uint16_t channel_id, double* data, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_double(channel_id, data, reason);
}

kelp_error_t com_send_char_blocking(uint16_t channel_id, char data, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_char(channel_id, data, reason);
}

kelp_error_t com_get_char_blocking(uint16_t channel_id, char* data, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_char(channel_id, data, reason);
}

kelp_error_t com_send_char_array_blocking(uint16_t channel_id, const char data[], uint32_t size, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_char_array(channel_id, data, size, reason);
}

kelp_error_t com_get_char_array_blocking(uint16_t channel_id, char* data, uint32_t max_size, uint32_t* size, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_char_array(channel_id, data, max_size, size, reason);
}

kelp_error_t com_send_char_array_fast_blocking(uint16_t channel_id, const char* data, uint16_t size, uint16_t reason) {
    com_channel_wait_until_writable(channel_id);

    return com_send_char_array_fast(channel_id, data, size, reason);
}

kelp_error_t com_get_char_array_fast_blocking(uint16_t channel_id, char (*data)[CHANNEL_SIZE], uint16_t* size, uint16_t* reason) {
    com_channel_wait_until_readable(channel_id);

    return com_get_char_array_fast(channel_id, data, size, reason);
}

kelp_error_t com_send_request_blocking(uint16_t channel_id, uint16_t request) {
    com_channel_wait_until_writable(channel_id);

    return com_send_request(channel_id, request);
}

kelp_error_t com_get_request_blocking(uint16_t channel_id, uint16_t* request) {
    com_channel_wait_until_readable(channel_id);

    return com_get_request(channel_id, request);
}