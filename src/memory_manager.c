#include "memory_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

MemoryTracker* g_memory_tracker = NULL;

MemoryPool* CreateMemoryPool(size_t initial_size) {
    MemoryPool* pool = malloc(sizeof(MemoryPool));
    if (!pool) return NULL;

    pool->buffer = malloc(initial_size);
    if (!pool->buffer) {
        free(pool);
        return NULL;
    }

    pool->size = initial_size;
    pool->used = 0;
    pool->peak_usage = 0;
    pool->next = NULL;

    return pool;
}

void FreeMemoryPool(MemoryPool* pool) {
    while (pool) {
        MemoryPool* next = pool->next;
        free(pool->buffer);
        free(pool);
        pool = next;
    }
}

void* PoolAlloc(MemoryPool* pool, size_t size) {
    if (!pool || size == 0) return NULL;

    size = (size + 7) & ~7;

    MemoryPool* current = pool;
    while (current) {
        if (current->used + size <= current->size) {
            void* ptr = current->buffer + current->used;
            current->used += size;
            if (current->used > current->peak_usage) {
                current->peak_usage = current->used;
            }
            return ptr;
        }

        if (!current->next) {

            size_t new_size = current->size * 2;
            if (new_size < size) new_size = size + 1024;

            current->next = CreateMemoryPool(new_size);
            if (!current->next) return NULL;
        }

        current = current->next;
    }

    return NULL;
}

void ResetMemoryPool(MemoryPool* pool) {
    while (pool) {
        pool->used = 0;
        pool = pool->next;
    }
}

size_t GetPoolUsage(MemoryPool* pool) {
    size_t total = 0;
    while (pool) {
        total += pool->used;
        pool = pool->next;
    }
    return total;
}

size_t GetPoolPeakUsage(MemoryPool* pool) {
    size_t peak = 0;
    while (pool) {
        if (pool->peak_usage > peak) {
            peak = pool->peak_usage;
        }
        pool = pool->next;
    }
    return peak;
}

StringBuffer* CreateStringBuffer(size_t initial_capacity) {
    StringBuffer* buffer = malloc(sizeof(StringBuffer));
    if (!buffer) return NULL;

    if (initial_capacity < 64) initial_capacity = 64;

    buffer->data = malloc(initial_capacity);
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }

    buffer->capacity = initial_capacity;
    buffer->length = 0;
    buffer->owns_memory = true;
    buffer->data[0] = '\0';

    return buffer;
}

void FreeStringBuffer(StringBuffer* buffer) {
    if (!buffer) return;

    if (buffer->owns_memory && buffer->data) {
        free(buffer->data);
    }
    free(buffer);
}

bool StringBufferReserve(StringBuffer* buffer, size_t capacity) {
    if (!buffer) return false;
    if (capacity <= buffer->capacity) return true;

    size_t new_capacity = buffer->capacity * 3 / 2;
    if (new_capacity < capacity) new_capacity = capacity;

    char* new_data = realloc(buffer->data, new_capacity);
    if (!new_data) return false;

    buffer->data = new_data;
    buffer->capacity = new_capacity;
    return true;
}

bool StringBufferAppend(StringBuffer* buffer, const char* str) {
    if (!buffer || !str) return false;

    size_t str_len = strlen(str);
    if (str_len == 0) return true;

    if (!StringBufferReserve(buffer, buffer->length + str_len + 1)) {
        return false;
    }

    memcpy(buffer->data + buffer->length, str, str_len);
    buffer->length += str_len;
    buffer->data[buffer->length] = '\0';

    return true;
}

bool StringBufferAppendChar(StringBuffer* buffer, char c) {
    if (!buffer) return false;

    if (!StringBufferReserve(buffer, buffer->length + 2)) {
        return false;
    }

    buffer->data[buffer->length] = c;
    buffer->length++;
    buffer->data[buffer->length] = '\0';

    return true;
}

void StringBufferClear(StringBuffer* buffer) {
    if (!buffer) return;

    buffer->length = 0;
    if (buffer->data) {
        buffer->data[0] = '\0';
    }
}

char* StringBufferDetach(StringBuffer* buffer) {
    if (!buffer || !buffer->data) return NULL;

    char* result = buffer->data;
    buffer->data = NULL;
    buffer->capacity = 0;
    buffer->length = 0;
    buffer->owns_memory = false;

    return result;
}

const char* StringBufferData(StringBuffer* buffer) {
    return buffer ? buffer->data : NULL;
}

size_t StringBufferLength(StringBuffer* buffer) {
    return buffer ? buffer->length : 0;
}

MemoryTracker* CreateMemoryTracker(void) {
    MemoryTracker* tracker = malloc(sizeof(MemoryTracker));
    if (!tracker) return NULL;

    tracker->capacity = 1024;
    tracker->allocations = malloc(tracker->capacity * sizeof(void*));
    tracker->sizes = malloc(tracker->capacity * sizeof(size_t));
    tracker->locations = malloc(tracker->capacity * sizeof(const char*));

    if (!tracker->allocations || !tracker->sizes || !tracker->locations) {
        free(tracker->allocations);
        free(tracker->sizes);
        free(tracker->locations);
        free(tracker);
        return NULL;
    }

    tracker->count = 0;
    tracker->total_allocated = 0;
    tracker->peak_allocated = 0;

    return tracker;
}

void FreeMemoryTracker(MemoryTracker* tracker) {
    if (!tracker) return;

    free(tracker->allocations);
    free(tracker->sizes);
    free(tracker->locations);
    free(tracker);
}

static bool ExpandTracker(MemoryTracker* tracker) {
    size_t new_capacity = tracker->capacity * 2;

    void** new_allocations = realloc(tracker->allocations, new_capacity * sizeof(void*));
    size_t* new_sizes = realloc(tracker->sizes, new_capacity * sizeof(size_t));
    const char** new_locations = realloc(tracker->locations, new_capacity * sizeof(const char*));

    if (!new_allocations || !new_sizes || !new_locations) {
        return false;
    }

    tracker->allocations = new_allocations;
    tracker->sizes = new_sizes;
    tracker->locations = new_locations;
    tracker->capacity = new_capacity;

    return true;
}

void* TrackedMalloc(MemoryTracker* tracker, size_t size, const char* location) {
    if (!tracker || size == 0) return NULL;

    void* ptr = malloc(size);
    if (!ptr) return NULL;

    if (tracker->count >= tracker->capacity) {
        if (!ExpandTracker(tracker)) {
            free(ptr);
            return NULL;
        }
    }

    tracker->allocations[tracker->count] = ptr;
    tracker->sizes[tracker->count] = size;
    tracker->locations[tracker->count] = location;
    tracker->count++;

    tracker->total_allocated += size;
    if (tracker->total_allocated > tracker->peak_allocated) {
        tracker->peak_allocated = tracker->total_allocated;
    }

    return ptr;
}

void* TrackedRealloc(MemoryTracker* tracker, void* ptr, size_t size, const char* location) {
    if (!tracker) return realloc(ptr, size);

    size_t old_size = 0;
    for (size_t i = 0; i < tracker->count; i++) {
        if (tracker->allocations[i] == ptr) {
            old_size = tracker->sizes[i];
            break;
        }
    }

    void* new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) return NULL;

    if (ptr) {

        for (size_t i = 0; i < tracker->count; i++) {
            if (tracker->allocations[i] == ptr) {

                for (size_t j = i; j < tracker->count - 1; j++) {
                    tracker->allocations[j] = tracker->allocations[j + 1];
                    tracker->sizes[j] = tracker->sizes[j + 1];
                    tracker->locations[j] = tracker->locations[j + 1];
                }
                tracker->count--;
                tracker->total_allocated -= old_size;
                break;
            }
        }
    }

    if (new_ptr && size > 0) {

        if (tracker->count >= tracker->capacity) {
            if (!ExpandTracker(tracker)) {
                return new_ptr;
            }
        }

        tracker->allocations[tracker->count] = new_ptr;
        tracker->sizes[tracker->count] = size;
        tracker->locations[tracker->count] = location;
        tracker->count++;

        tracker->total_allocated += size;
        if (tracker->total_allocated > tracker->peak_allocated) {
            tracker->peak_allocated = tracker->total_allocated;
        }
    }

    return new_ptr;
}

void TrackedFree(MemoryTracker* tracker, void* ptr) {
    if (!ptr) return;

    if (tracker) {

        for (size_t i = 0; i < tracker->count; i++) {
            if (tracker->allocations[i] == ptr) {
                tracker->total_allocated -= tracker->sizes[i];

                for (size_t j = i; j < tracker->count - 1; j++) {
                    tracker->allocations[j] = tracker->allocations[j + 1];
                    tracker->sizes[j] = tracker->sizes[j + 1];
                    tracker->locations[j] = tracker->locations[j + 1];
                }
                tracker->count--;
                break;
            }
        }
    }

    free(ptr);
}

void PrintMemoryReport(MemoryTracker* tracker) {
    if (!tracker) return;

    printf("=== Memory Report ===\n");
    printf("Current allocations: %zu\n", tracker->count);
    printf("Current memory usage: %zu bytes\n", tracker->total_allocated);
    printf("Peak memory usage: %zu bytes\n", tracker->peak_allocated);

    if (tracker->count > 0) {
        printf("\nActive allocations:\n");
        for (size_t i = 0; i < tracker->count; i++) {
            printf("  %p: %zu bytes at %s\n",
                   tracker->allocations[i],
                   tracker->sizes[i],
                   tracker->locations[i] ? tracker->locations[i] : "unknown");
        }
    }
    printf("==================\n");
}

bool HasMemoryLeaks(MemoryTracker* tracker) {
    return tracker && tracker->count > 0;
}

void InitGlobalMemoryTracking(void) {
    if (!g_memory_tracker) {
        g_memory_tracker = CreateMemoryTracker();
    }
}

void CleanupGlobalMemoryTracking(void) {
    if (g_memory_tracker) {
        if (HasMemoryLeaks(g_memory_tracker)) {
            printf("WARNING: Memory leaks detected!\n");
            PrintMemoryReport(g_memory_tracker);
        }
        FreeMemoryTracker(g_memory_tracker);
        g_memory_tracker = NULL;
    }
}