#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>
#include <stdbool.h>

typedef struct MemoryPool {
    char* buffer;
    size_t size;
    size_t used;
    size_t peak_usage;
    struct MemoryPool* next;
} MemoryPool;

typedef struct StringBuffer {
    char* data;
    size_t capacity;
    size_t length;
    bool owns_memory;
} StringBuffer;

typedef struct MemoryTracker {
    void** allocations;
    size_t* sizes;
    const char** locations;
    size_t count;
    size_t capacity;
    size_t total_allocated;
    size_t peak_allocated;
} MemoryTracker;

MemoryPool* CreateMemoryPool(size_t initial_size);
void FreeMemoryPool(MemoryPool* pool);
void* PoolAlloc(MemoryPool* pool, size_t size);
void ResetMemoryPool(MemoryPool* pool);
size_t GetPoolUsage(MemoryPool* pool);
size_t GetPoolPeakUsage(MemoryPool* pool);

StringBuffer* CreateStringBuffer(size_t initial_capacity);
void FreeStringBuffer(StringBuffer* buffer);
bool StringBufferAppend(StringBuffer* buffer, const char* str);
bool StringBufferAppendChar(StringBuffer* buffer, char c);
bool StringBufferReserve(StringBuffer* buffer, size_t capacity);
void StringBufferClear(StringBuffer* buffer);
char* StringBufferDetach(StringBuffer* buffer);
const char* StringBufferData(StringBuffer* buffer);
size_t StringBufferLength(StringBuffer* buffer);

MemoryTracker* CreateMemoryTracker(void);
void FreeMemoryTracker(MemoryTracker* tracker);
void* TrackedMalloc(MemoryTracker* tracker, size_t size, const char* location);
void* TrackedRealloc(MemoryTracker* tracker, void* ptr, size_t size, const char* location);
void TrackedFree(MemoryTracker* tracker, void* ptr);
void PrintMemoryReport(MemoryTracker* tracker);
bool HasMemoryLeaks(MemoryTracker* tracker);

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define TRACKED_MALLOC(tracker, size) TrackedMalloc(tracker, size, __FILE__ ":" TOSTRING(__LINE__))
#define TRACKED_REALLOC(tracker, ptr, size) TrackedRealloc(tracker, ptr, size, __FILE__ ":" TOSTRING(__LINE__))
#define TRACKED_FREE(tracker, ptr) TrackedFree(tracker, ptr)

extern MemoryTracker* g_memory_tracker;

void InitGlobalMemoryTracking(void);
void CleanupGlobalMemoryTracking(void);

#endif