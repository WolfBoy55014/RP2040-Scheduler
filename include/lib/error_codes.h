//
// Created by wolfboy on 5/1/2026.
//

#ifndef KELPOS_LITE_ERROR_CODES_H
#define KELPOS_LITE_ERROR_CODES_H

// TODO: Apply to channel functions
// TODO: Apply to protocol functions
// TODO: Apply to scheduler functions
// TODO: Apply to service functions
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
    KELP_INVALID_ID = -10,
    KELP_NO_TASK = -11,
    KELP_NOT_OWNER = -12,
    KELP_UNALLOCATED = -13,
    KELP_ALLOCATED = -14,
    KELP_NONE_FREE = -15,
    KELP_NOT_CONNECTED = -16,
    KELP_WRONG_REASON = -17,
    KELP_IO = -18,
    KELP_NO_EXIST = -19,
} kelp_error_t;

#define KELP_RETURN_ON_ERROR(error) if ((error) != KELP_OK) {return (error);}

#endif //KELPOS_LITE_ERROR_CODES_H