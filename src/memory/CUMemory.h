#ifndef CU_MEMORY_H
#define CU_MEMORY_H

#include <stddef.h>

/* Size in bytes of the static heap used by the memory middleware. */
#ifndef CUTEST_MEMORY_HEAP_SIZE
#define CUTEST_MEMORY_HEAP_SIZE 8192
#endif

/* Alignment applied to middleware-managed allocations. */
#ifndef CUTEST_MEMORY_ALIGNMENT
#define CUTEST_MEMORY_ALIGNMENT sizeof(void*)
#endif

/* Optional lock hook used to protect allocator critical sections. */
#ifndef CUTEST_MEMORY_LOCK
#define CUTEST_MEMORY_LOCK() ((void)0)
#endif

/* Optional unlock hook paired with CUTEST_MEMORY_LOCK(). */
#ifndef CUTEST_MEMORY_UNLOCK
#define CUTEST_MEMORY_UNLOCK() ((void)0)
#endif

/* Allocate a payload block from the middleware heap. */
void* CuMemoryMalloc(size_t size);
/* Allocate and zero-initialize a payload block from the middleware heap. */
void* CuMemoryCalloc(size_t count, size_t size);
/* Resize a middleware allocation while preserving existing contents. */
void* CuMemoryRealloc(void* ptr, size_t size);
/* Release a previously allocated middleware block. */
void CuMemoryFree(void* ptr);
/* Reinitialize the middleware heap and discard outstanding allocations. */
void CuMemoryReset(void);
/* Return the currently available free space in the middleware heap. */
size_t CuMemoryGetFreeSize(void);
/* Return the minimum free-space watermark since the last reset. */
size_t CuMemoryGetMinimumEverFreeSize(void);

#endif /* CU_MEMORY_H */
