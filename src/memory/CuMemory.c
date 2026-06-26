#include <stdint.h>
#include <string.h>

#include "memory/CUMemory.h"

typedef struct CuMemoryBlock CuMemoryBlock;

struct CuMemoryBlock {
    size_t size;
    CuMemoryBlock* next_free;
};

static unsigned char cuMemoryHeapStorage[CUTEST_MEMORY_HEAP_SIZE +
                                         CUTEST_MEMORY_ALIGNMENT];
static CuMemoryBlock* cuMemoryFreeList = NULL;
static size_t cuMemoryAlignment = 0;
static size_t cuMemoryHeaderSize = 0;
static size_t cuMemoryMinimumSplitSize = 0;
static size_t cuMemoryFreeBytesRemaining = 0;
static size_t cuMemoryMinimumEverFreeBytesRemaining = 0;
static int cuMemoryInitialized = 0;

static size_t CuMemoryGetEffectiveAlignment(void) {
    size_t alignment = (size_t)CUTEST_MEMORY_ALIGNMENT;

    if (alignment == 0) {
        alignment = sizeof(void*);
    }
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }

    return alignment;
}

static size_t CuMemoryAlignUp(size_t value) {
    size_t alignment = cuMemoryAlignment;
    size_t remainder;

    if (alignment == 0) {
        return value;
    }

    remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }
    if (value > SIZE_MAX - (alignment - remainder)) {
        return 0;
    }

    return value + (alignment - remainder);
}

static unsigned char* CuMemoryBlockToPayload(CuMemoryBlock* block) {
    return ((unsigned char*)block) + cuMemoryHeaderSize;
}

static CuMemoryBlock* CuMemoryPayloadToBlock(void* ptr) {
    return (CuMemoryBlock*)(((unsigned char*)ptr) - cuMemoryHeaderSize);
}

static size_t CuMemoryBlockPayloadSize(const CuMemoryBlock* block) {
    if (block->size < cuMemoryHeaderSize) {
        return 0;
    }

    return block->size - cuMemoryHeaderSize;
}

static void CuMemoryInsertBlock(CuMemoryBlock* block) {
    CuMemoryBlock* previous = NULL;
    CuMemoryBlock* current = cuMemoryFreeList;

    while (current != NULL && current < block) {
        previous = current;
        current = current->next_free;
    }

    block->next_free = current;
    if (previous == NULL) {
        cuMemoryFreeList = block;
    } else {
        previous->next_free = block;
    }

    if (current != NULL &&
        ((unsigned char*)block + block->size) == (unsigned char*)current) {
        block->size += current->size;
        block->next_free = current->next_free;
    }

    if (previous != NULL &&
        ((unsigned char*)previous + previous->size) == (unsigned char*)block) {
        previous->size += block->size;
        previous->next_free = block->next_free;
    }
}

static int CuMemoryGetRequestSize(size_t payloadSize, size_t* totalSize) {
    size_t requestSize;

    if (payloadSize == 0) {
        return 0;
    }
    if (payloadSize > SIZE_MAX - cuMemoryHeaderSize) {
        return 0;
    }

    requestSize = CuMemoryAlignUp(cuMemoryHeaderSize + payloadSize);
    if (requestSize == 0 || requestSize < cuMemoryHeaderSize) {
        return 0;
    }

    *totalSize = requestSize;
    return 1;
}

static void CuMemoryInitializeHeap(void) {
    uintptr_t rawAddress;
    uintptr_t alignedAddress;
    unsigned char* heapStart;
    size_t heapSize = (size_t)CUTEST_MEMORY_HEAP_SIZE;

    cuMemoryAlignment = CuMemoryGetEffectiveAlignment();
    cuMemoryHeaderSize = CuMemoryAlignUp(sizeof(CuMemoryBlock));
    cuMemoryMinimumSplitSize = CuMemoryAlignUp(cuMemoryHeaderSize + 1);

    if (cuMemoryHeaderSize == 0 || cuMemoryMinimumSplitSize == 0) {
        cuMemoryFreeList = NULL;
        cuMemoryFreeBytesRemaining = 0;
        cuMemoryMinimumEverFreeBytesRemaining = 0;
        cuMemoryInitialized = 1;
        return;
    }

    rawAddress = (uintptr_t)&cuMemoryHeapStorage[0];
    alignedAddress = rawAddress;
    if (cuMemoryAlignment > 0) {
        size_t remainder = (size_t)(alignedAddress % cuMemoryAlignment);
        if (remainder != 0) {
            alignedAddress += (uintptr_t)(cuMemoryAlignment - remainder);
        }
    }
    heapStart = (unsigned char*)alignedAddress;

    cuMemoryFreeList = (CuMemoryBlock*)heapStart;
    cuMemoryFreeList->size = heapSize;
    cuMemoryFreeList->next_free = NULL;

    cuMemoryFreeBytesRemaining = heapSize;
    cuMemoryMinimumEverFreeBytesRemaining = heapSize;
    cuMemoryInitialized = 1;
}

static void CuMemoryEnsureInitialized(void) {
    if (!cuMemoryInitialized) {
        CuMemoryInitializeHeap();
    }
}

void* CuMemoryMalloc(size_t size) {
    CuMemoryBlock* previous = NULL;
    CuMemoryBlock* block = NULL;
    size_t requestSize = 0;

    CuMemoryEnsureInitialized();
    if (!CuMemoryGetRequestSize(size, &requestSize)) {
        return NULL;
    }

    CUTEST_MEMORY_LOCK();

    for (block = cuMemoryFreeList; block != NULL; block = block->next_free) {
        if (block->size >= requestSize) {
            break;
        }
        previous = block;
    }

    if (block == NULL) {
        CUTEST_MEMORY_UNLOCK();
        return NULL;
    }

    if (block->size - requestSize >= cuMemoryMinimumSplitSize) {
        CuMemoryBlock* remainder =
            (CuMemoryBlock*)(((unsigned char*)block) + requestSize);

        remainder->size = block->size - requestSize;
        remainder->next_free = block->next_free;

        if (previous == NULL) {
            cuMemoryFreeList = remainder;
        } else {
            previous->next_free = remainder;
        }

        block->size = requestSize;
    } else if (previous == NULL) {
        cuMemoryFreeList = block->next_free;
    } else {
        previous->next_free = block->next_free;
    }

    block->next_free = NULL;
    cuMemoryFreeBytesRemaining -= block->size;
    if (cuMemoryFreeBytesRemaining < cuMemoryMinimumEverFreeBytesRemaining) {
        cuMemoryMinimumEverFreeBytesRemaining = cuMemoryFreeBytesRemaining;
    }

    CUTEST_MEMORY_UNLOCK();

    return CuMemoryBlockToPayload(block);
}

void* CuMemoryCalloc(size_t count, size_t size) {
    void* block;
    size_t totalSize;

    if (count == 0 || size == 0) {
        return NULL;
    }
    if (count > SIZE_MAX / size) {
        return NULL;
    }

    totalSize = count * size;
    block = CuMemoryMalloc(totalSize);
    if (block != NULL) {
        memset(block, 0, totalSize);
    }

    return block;
}

void* CuMemoryRealloc(void* ptr, size_t size) {
    CuMemoryBlock* block;
    CuMemoryBlock* previous;
    CuMemoryBlock* next;
    size_t requestSize = 0;
    size_t oldBlockSize;
    size_t oldPayloadSize;
    void* newPtr;

    if (ptr == NULL) {
        return CuMemoryMalloc(size);
    }
    if (size == 0) {
        CuMemoryFree(ptr);
        return NULL;
    }

    CuMemoryEnsureInitialized();
    if (!CuMemoryGetRequestSize(size, &requestSize)) {
        return NULL;
    }

    block = CuMemoryPayloadToBlock(ptr);
    oldBlockSize = block->size;
    oldPayloadSize = CuMemoryBlockPayloadSize(block);

    CUTEST_MEMORY_LOCK();

    if (requestSize <= oldBlockSize) {
        size_t released = oldBlockSize - requestSize;

        if (released >= cuMemoryMinimumSplitSize) {
            CuMemoryBlock* remainder =
                (CuMemoryBlock*)(((unsigned char*)block) + requestSize);

            block->size = requestSize;
            remainder->size = released;
            remainder->next_free = NULL;
            CuMemoryInsertBlock(remainder);
            cuMemoryFreeBytesRemaining += released;
        }

        CUTEST_MEMORY_UNLOCK();
        return ptr;
    }

    previous = NULL;
    next = cuMemoryFreeList;
    while (next != NULL && next < block) {
        previous = next;
        next = next->next_free;
    }

    if (next != NULL &&
        ((unsigned char*)block + oldBlockSize) == (unsigned char*)next &&
        oldBlockSize + next->size >= requestSize) {
        size_t combinedSize = oldBlockSize + next->size;
        size_t grownBy;

        if (previous == NULL) {
            cuMemoryFreeList = next->next_free;
        } else {
            previous->next_free = next->next_free;
        }

        block->size = combinedSize;
        if (combinedSize - requestSize >= cuMemoryMinimumSplitSize) {
            CuMemoryBlock* remainder =
                (CuMemoryBlock*)(((unsigned char*)block) + requestSize);

            block->size = requestSize;
            remainder->size = combinedSize - requestSize;
            remainder->next_free = NULL;
            CuMemoryInsertBlock(remainder);
        }

        grownBy = block->size - oldBlockSize;
        cuMemoryFreeBytesRemaining -= grownBy;
        if (cuMemoryFreeBytesRemaining < cuMemoryMinimumEverFreeBytesRemaining) {
            cuMemoryMinimumEverFreeBytesRemaining = cuMemoryFreeBytesRemaining;
        }

        CUTEST_MEMORY_UNLOCK();
        return ptr;
    }

    CUTEST_MEMORY_UNLOCK();

    newPtr = CuMemoryMalloc(size);
    if (newPtr == NULL) {
        return NULL;
    }

    memcpy(newPtr, ptr, oldPayloadSize < size ? oldPayloadSize : size);
    CuMemoryFree(ptr);
    return newPtr;
}

void CuMemoryFree(void* ptr) {
    CuMemoryBlock* block;

    if (ptr == NULL) {
        return;
    }

    CuMemoryEnsureInitialized();
    block = CuMemoryPayloadToBlock(ptr);

    CUTEST_MEMORY_LOCK();
    cuMemoryFreeBytesRemaining += block->size;
    block->next_free = NULL;
    CuMemoryInsertBlock(block);
    CUTEST_MEMORY_UNLOCK();
}

void CuMemoryReset(void) {
    CUTEST_MEMORY_LOCK();
    cuMemoryInitialized = 0;
    CuMemoryInitializeHeap();
    CUTEST_MEMORY_UNLOCK();
}

size_t CuMemoryGetFreeSize(void) {
    size_t freeSize;

    CuMemoryEnsureInitialized();
    CUTEST_MEMORY_LOCK();
    freeSize = cuMemoryFreeBytesRemaining;
    CUTEST_MEMORY_UNLOCK();
    return freeSize;
}

size_t CuMemoryGetMinimumEverFreeSize(void) {
    size_t freeSize;

    CuMemoryEnsureInitialized();
    CUTEST_MEMORY_LOCK();
    freeSize = cuMemoryMinimumEverFreeBytesRemaining;
    CUTEST_MEMORY_UNLOCK();
    return freeSize;
}
