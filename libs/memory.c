/*
 * memory.c — custom memory allocator using mmap().
 *
 * Instead of malloc/free from <stdlib.h>, we ask the operating system
 * directly for memory using the mmap() system call. This maps a chunk
 * of virtual memory that we can read and write to.
 *
 * mmap() is what malloc() uses internally — we just skip the middleman.
 */

#include <sys/mman.h>

/*
 * Allocate 'size' bytes of memory from the OS.
 * MAP_PRIVATE = our own private copy.
 * MAP_ANON    = not backed by a file, just empty memory.
 * Returns NULL (0) on failure.
 */
void *mem_alloc(unsigned long size) {
    void *p = mmap(0, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON, -1, 0);

    if (p == MAP_FAILED)
        return 0;
    return p;
}

/* Return memory back to the OS. Must pass the same size used to allocate. */
void mem_free(void *ptr, unsigned long size) {
    munmap(ptr, size);
}
