//
// Created by wolfboy on 4/6/2026.
//

#ifndef KELPOS_LITE_MEMORY_ALLOCATION_H
#define KELPOS_LITE_MEMORY_ALLOCATION_H

#include <stddef.h>
#include "kernel_config.h"

void* aligned_realloc(void* ptr, size_t alignment, size_t size);

#if ALIGN_STACK_ALLOCATIONS
#define stack_alloc(size) aligned_alloc(STACK_ALIGNMENT_SIZE, size)
#define stack_realloc(ptr, size) aligned_realloc(ptr, STACK_ALIGNMENT_SIZE, size)
#define stack_free(ptr) free(ptr)
#else
#define stack_alloc(size) malloc(size)
#define stack_realloc(ptr, size) realloc(ptr, size)
#define stack_free(ptr) free(ptr)
#endif

#endif //KELPOS_LITE_MEMORY_ALLOCATION_H
