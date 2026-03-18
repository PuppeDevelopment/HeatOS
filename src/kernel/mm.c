#include "mm.h"

#include "string.h"

static uint8_t* pmm_bitmap;
static uint32_t pmm_max_blocks;
static uint32_t pmm_used_blocks;
static uint32_t pmm_reserved_blocks;

typedef struct heap_block {
    uint32_t size;
    struct heap_block* next;
    uint8_t free;
    uint8_t _pad[3];
} heap_block_t;

static heap_block_t* heap_head = 0;
static bool heap_ready = false;

static uint32_t heap_align8(uint32_t value) {
    return (value + 7u) & ~7u;
}

static void heap_init(void) {
    heap_head = (heap_block_t*)(uintptr_t)HEAP_START;
    heap_head->size = HEAP_SIZE - (uint32_t)sizeof(heap_block_t);
    heap_head->next = 0;
    heap_head->free = 1;
    heap_ready = true;
}

static void heap_coalesce(void) {
    heap_block_t* block = heap_head;

    while (block && block->next) {
        uint8_t* block_end = (uint8_t*)block + sizeof(heap_block_t) + block->size;
        if (block->free && block->next->free && block_end == (uint8_t*)block->next) {
            block->size += (uint32_t)sizeof(heap_block_t) + block->next->size;
            block->next = block->next->next;
            continue;
        }
        block = block->next;
    }
}

static void pmm_bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}
static void pmm_bitmap_clear(uint32_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}
static bool pmm_bitmap_test(uint32_t bit) {
    return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(uint32_t mem_size) {
    uint32_t bitmap_bytes;
    uint32_t reserve_bytes;

    if (mem_size < PAGE_SIZE) mem_size = PAGE_SIZE;

    pmm_max_blocks = mem_size / PAGE_SIZE;

    // Keep low memory + kernel heap region permanently reserved.
    reserve_bytes = HEAP_START + HEAP_SIZE;
    if (reserve_bytes > mem_size) reserve_bytes = mem_size;
    pmm_reserved_blocks = reserve_bytes / PAGE_SIZE;
    if (pmm_reserved_blocks > pmm_max_blocks) pmm_reserved_blocks = pmm_max_blocks;

    // Bitmap location is fixed for this kernel stage.
    pmm_bitmap = (uint8_t*)0x100000;
    bitmap_bytes = (pmm_max_blocks + 7u) / 8u;

    memset(pmm_bitmap, 0xFF, bitmap_bytes);
    for (uint32_t i = pmm_reserved_blocks; i < pmm_max_blocks; i++) {
        pmm_bitmap_clear(i);
    }

    pmm_used_blocks = pmm_reserved_blocks;
    heap_init();
}

void* pmm_alloc_page(void) {
    for (uint32_t i = pmm_reserved_blocks; i < pmm_max_blocks; i++) {
        if (!pmm_bitmap_test(i)) {
            pmm_bitmap_set(i);
            pmm_used_blocks++;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return 0;
}

void pmm_free_page(void* p) {
    if (!p) return;

    uint32_t block = (uint32_t)p / PAGE_SIZE;
    if (block < pmm_reserved_blocks || block >= pmm_max_blocks) return;
    if (!pmm_bitmap_test(block)) return;

    pmm_bitmap_clear(block);
    if (pmm_used_blocks > 0) pmm_used_blocks--;
}

void* kmalloc(uint32_t size) {
    heap_block_t* block;
    uint32_t need;

    if (!heap_ready || size == 0) return 0;

    need = heap_align8(size);
    if (need == 0 || need > HEAP_SIZE) return 0;

    block = heap_head;
    while (block) {
        if (block->free && block->size >= need) {
            if (block->size >= need + (uint32_t)sizeof(heap_block_t) + 8u) {
                heap_block_t* split = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + need);
                split->size = block->size - need - (uint32_t)sizeof(heap_block_t);
                split->next = block->next;
                split->free = 1;

                block->next = split;
                block->size = need;
            }

            block->free = 0;
            return (void*)(block + 1);
        }
        block = block->next;
    }

    return 0;
}

void kfree(void* p) {
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t addr;
    heap_block_t* block;

    if (!heap_ready || !p) return;

    heap_start = (uintptr_t)HEAP_START;
    heap_end = heap_start + (uintptr_t)HEAP_SIZE;
    addr = (uintptr_t)p;

    if (addr < heap_start + (uintptr_t)sizeof(heap_block_t) || addr >= heap_end) {
        return;
    }

    block = ((heap_block_t*)p) - 1;
    if ((uintptr_t)block < heap_start || (uintptr_t)block >= heap_end) {
        return;
    }

    block->free = 1;
    heap_coalesce();
}

uint32_t pmm_total_blocks(void) {
    return pmm_max_blocks;
}

uint32_t pmm_used_blocks_count(void) {
    return pmm_used_blocks;
}

uint32_t pmm_free_blocks_count(void) {
    if (pmm_max_blocks < pmm_used_blocks) return 0;
    return pmm_max_blocks - pmm_used_blocks;
}

uint32_t kheap_used_bytes(void) {
    uint32_t used = 0;
    heap_block_t* block = heap_head;

    if (!heap_ready) return 0;

    while (block) {
        if (!block->free) {
            used += block->size;
        }
        block = block->next;
    }

    return used;
}

uint32_t kheap_free_bytes(void) {
    uint32_t free_bytes = 0;
    heap_block_t* block = heap_head;

    if (!heap_ready) return 0;

    while (block) {
        if (block->free) {
            free_bytes += block->size;
        }
        block = block->next;
    }

    return free_bytes;
}
