#ifndef MEMORY_H
#define MEMORY_H

void *mem_alloc(unsigned long size);
void mem_free(void *ptr, unsigned long size);

#endif
