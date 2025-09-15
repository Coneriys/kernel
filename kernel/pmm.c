#include "../include/memory.h"

static uint32_t* bitmap;
static size_t bitmap_size;
static size_t total_pages;
static size_t free_pages;
static uintptr_t memory_start = 0;
static uint64_t total_memory_bytes = 0; // Support for >4GB memory

extern void terminal_writestring(const char* data);

static void set_bit(uint32_t bit) {
    bitmap[bit / 32] |= (1 << (bit % 32));
}

static void clear_bit(uint32_t bit) {
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

static int test_bit(uint32_t bit) {
    return bitmap[bit / 32] & (1 << (bit % 32));
}

static uint32_t find_free_page(void) {
    for (size_t i = 0; i < total_pages; i++) {
        if (!test_bit(i)) {
            return i;
        }
    }
    return (uint32_t)-1;
}

void pmm_init(multiboot_info_t* mbi) {
    terminal_writestring("Initializing Physical Memory Manager...\n");
    
    if (!mbi || !(mbi->flags & (1 << 6))) {
        terminal_writestring("No multiboot memory map, using fallback allocation\n");
        
        // Fallback: assume 128MB of RAM starting at 1MB
        memory_start = 0x100000;  // 1MB
        size_t total_memory = 128 * 1024 * 1024;  // 128MB
        total_pages = total_memory / 4096;
        free_pages = total_pages;
        
        // Use first 64KB after 1MB for bitmap
        bitmap = (uint32_t*)memory_start;
        bitmap_size = (total_pages + 31) / 32;
        
        // Clear bitmap (all pages free)
        for (size_t i = 0; i < bitmap_size; i++) {
            bitmap[i] = 0;
        }
        
        // Mark first few pages as used (kernel + bitmap)
        for (size_t i = 0; i < 64; i++) {  // Reserve first 256KB
            set_bit(i);
            free_pages--;
        }
        
        terminal_writestring("PMM initialized with fallback mode (128MB)\n");
        return;
    }
    
    size_t total_memory = 0;
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    
    while ((uintptr_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (mmap->type == 1) {
            total_memory += mmap->length;
            if (memory_start == 0 && mmap->base_addr >= 0x200000) {
                memory_start = mmap->base_addr;
            }
        }
        mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size));
    }
    
    if (memory_start == 0 || total_memory == 0) {
        memory_start = 0x200000;
        total_memory = 16 * 1024 * 1024;
    }
    
    total_pages = total_memory / PAGE_SIZE;
    bitmap_size = (total_pages + 31) / 32;
    
    bitmap = (uint32_t*)(memory_start);
    memory_start += bitmap_size * sizeof(uint32_t);
    memory_start = PAGE_ALIGN(memory_start);
    
    free_pages = 0;
    
    for (size_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFFFFFFFF;
    }
    
    size_t available_pages = (total_memory - (memory_start - 0x200000)) / PAGE_SIZE;
    if (available_pages > total_pages) {
        available_pages = total_pages;
    }
    
    for (size_t i = 0; i < available_pages && i < total_pages; i++) {
        clear_bit(i);
        free_pages++;
    }
    
    terminal_writestring("PMM initialized successfully\n");
    
    terminal_writestring("Debug: PMM stats\n");
}

void* pmm_alloc_page(void) {
    // Quick check for available pages
    if (free_pages == 0) {
        return NULL; // Silent fail to prevent spam
    }
    
    uint32_t page = find_free_page();
    if (page == (uint32_t)-1) {
        return NULL; // Silent fail to prevent spam
    }
    
    set_bit(page);
    free_pages--;
    
    return (void*)(memory_start + page * PAGE_SIZE);
}

void pmm_free_page(void* page) {
    if (!page) return;
    
    uintptr_t addr = (uintptr_t)page;
    if (addr < memory_start) return;
    
    uint32_t page_num = (addr - memory_start) / PAGE_SIZE;
    if (page_num >= total_pages) return;
    
    clear_bit(page_num);
    free_pages++;
}

size_t pmm_get_total_memory(void) {
    return total_pages * PAGE_SIZE;
}

size_t pmm_get_free_memory(void) {
    return free_pages * PAGE_SIZE;
}