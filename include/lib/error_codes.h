//
// Created by wolfboy on 5/1/2026.
//

#ifndef KELPOS_LITE_ERROR_CODES_H
#define KELPOS_LITE_ERROR_CODES_H

// TODO: Apply to channel functions
// TODO: Apply to protocol functions
// TODO: Apply to scheduler functions
// TODO: Apply to service functions
typedef enum {
    KELP_OK,
    KELP_ERROR,
    KELP_MEMORY,
    KELP_ID_TAKEN,
    KELP_PROTOCOL,
    KELP_TIMEOUT,
    KELP_WRONG_TYPE,
    KELP_CHANNEL_EMPTY,
    KELP_CHANNEL_FULL,
    KELP_TOO_BIG,
    KELP_INVALID_ID,
    KELP_NO_TASK,
    KELP_NOT_OWNER,
    KELP_UNALLOCATED,
    KELP_ALLOCATED,
    KELP_NONE_FREE,
    KELP_NOT_CONNECTED,
} kelp_error_t;

#endif //KELPOS_LITE_ERROR_CODES_H