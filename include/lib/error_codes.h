//
// Created by wolfboy on 5/1/2026.
//

#ifndef KELPOS_LITE_ERROR_CODES_H
#define KELPOS_LITE_ERROR_CODES_H

#include <stdint.h>

typedef enum : int32_t {
    KELP_OK = 0,
    KELP_ERROR = -1,
    KELP_MEMORY = -2,
    KELP_ID_TAKEN = -3,
    KELP_PROTOCOL = -4,
    KELP_TIMEOUT = -5,
    KELP_WRONG_TYPE = -6,
    KELP_CHANNEL_EMPTY = -7,
    KELP_CHANNEL_FULL = -8,
    KELP_TOO_BIG = -9,
    KELP_TOO_SMALL = -10,
    KELP_INVALID = -11,
    KELP_NO_TASK = -12,
    KELP_NOT_OWNER = -13,
    KELP_UNALLOCATED = -14,
    KELP_ALLOCATED = -15,
    KELP_NONE_FREE = -16,
    KELP_NOT_CONNECTED = -17,
    KELP_WRONG_REASON = -18,
    KELP_IO = -19,
    KELP_NO_EXIST = -20,
    KELP_EXISTS = -21,
    KELP_BUSY = -22,
    KELP_DENIED = -23,
    KELP_PROTECTED = -24,
    KELP_CORRUPT = -25,
    KELP_LOCKED = -26,
} kelp_error_t;

#define KELP_RETURN_ON_ERROR(error) if ((error) != KELP_OK) {return (error);}

#endif //KELPOS_LITE_ERROR_CODES_H