//
// Created by wolfboy on 4/6/2026.
//

#include "../include/memory/memory_allocation.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void* aligned_realloc(void* ptr, const size_t alignment, const size_t size) {
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return aligned_alloc(alignment, size);
    }

    void* temp = realloc(ptr, size);

    // first, check if NULL (no memory left)
    if (temp == NULL) {
        return NULL;
    }

    // check if the memory is still aligned
    if (((uintptr_t)temp & (alignment - 1)) == 0) {
        return temp;
    }

    // otherwise, make new aligned memory.
    // realloc does not preserve alignment.
    // aligned_alloc more memory, copy the contents, and free temp
    // (ptr was already freed by `realloc`)
    void* aligned_memory = aligned_alloc(alignment, size);
    if (aligned_memory == NULL) {
        free(temp);
        return NULL;
    }
    memcpy(aligned_memory, temp, size);
    free(temp);
    return aligned_memory;
}
