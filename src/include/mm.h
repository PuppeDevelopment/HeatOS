#ifndef MM_H
#define MM_H

#include "types.h"

#define PAGE_SIZE 4096
#define HEAP_START 0x1000000
#define HEAP_SIZE  0x1000000

void pmm_init(uint32_t mem_size);
void* pmm_alloc_page(void);
void pmm_free_page(void* p);
uint32_t pmm_total_blocks(void);
uint32_t pmm_used_blocks_count(void);
uint32_t pmm_free_blocks_count(void);

void* kmalloc(uint32_t size);
void kfree(void* p);
uint32_t kheap_used_bytes(void);
uint32_t kheap_free_bytes(void);

#endif
