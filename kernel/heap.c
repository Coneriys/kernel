#include "../include/memory.h"

typedef struct heap_block {
    size_t size;
    int free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_start = NULL;
static void* heap_end = NULL;

extern void terminal_writestring(const char* data);
extern void* pmm_alloc_page(void);
extern void pmm_free_page(void* page);

void heap_init(void) {
    terminal_writestring("Initializing heap...\n");
    heap_start = (heap_block_t*)pmm_alloc_page();
    if (!heap_start) {
        terminal_writestring("ERROR: Failed to allocate initial heap page\n");
        terminal_writestring("Falling back to static heap allocation\n");
        
        static char static_heap[16384];
        heap_start = (heap_block_t*)static_heap;
        heap_start->size = sizeof(static_heap) - sizeof(heap_block_t);
        heap_start->free = 1;
        heap_start->next = NULL;
        heap_end = (void*)((uintptr_t)heap_start + sizeof(static_heap));
        
        terminal_writestring("Static heap initialized successfully\n");
        return;
    }
    
    heap_start->size = PAGE_SIZE - sizeof(heap_block_t);
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_end = (void*)((uintptr_t)heap_start + PAGE_SIZE);
    
    terminal_writestring("Dynamic heap initialized successfully\n");
}

static heap_block_t* find_free_block(size_t size) {
    heap_block_t* current = heap_start;
    
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

static void split_block(heap_block_t* block, size_t size) {
    if (block->size > size + sizeof(heap_block_t) + 16) {
        heap_block_t* new_block = (heap_block_t*)((uintptr_t)block + sizeof(heap_block_t) + size);
        new_block->size = block->size - size - sizeof(heap_block_t);
        new_block->free = 1;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}

static void merge_free_blocks(void) {
    heap_block_t* current = heap_start;
    
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + sizeof(heap_block_t);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size = (size + 7) & ~7;
    
    heap_block_t* block = find_free_block(size);
    
    if (!block) {
        size_t pages_needed = (size + sizeof(heap_block_t) + PAGE_SIZE - 1) / PAGE_SIZE;
        
        for (size_t i = 0; i < pages_needed; i++) {
            void* new_page = pmm_alloc_page();
            if (!new_page) {
                return NULL;
            }
            
            if (i == 0) {
                block = (heap_block_t*)new_page;
                block->size = pages_needed * PAGE_SIZE - sizeof(heap_block_t);
                block->free = 1;
                block->next = NULL;
                
                heap_block_t* current = heap_start;
                while (current->next) {
                    current = current->next;
                }
                current->next = block;
            }
        }
    }
    
    if (block) {
        split_block(block, size);
        block->free = 0;
        return (void*)((uintptr_t)block + sizeof(heap_block_t));
    }
    
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((uintptr_t)ptr - sizeof(heap_block_t));
    
    if (block < heap_start || (uintptr_t)block >= (uintptr_t)heap_end) {
        return;
    }
    
    block->free = 1;
    merge_free_blocks();
}