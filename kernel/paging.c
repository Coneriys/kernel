#include "../include/paging.h"
#include "../include/memory.h"
#include <stddef.h>

extern void terminal_writestring(const char* data);
extern void* pmm_alloc_page(void);
extern void pmm_free_page(void* page);

static page_directory_t* kernel_directory = NULL;
static page_directory_t* current_directory = NULL;

extern void load_page_directory(uint32_t);
extern void enable_paging_asm(void);

static uint32_t get_page_frame(void* addr) {
    return ((uint32_t)addr) >> 12;
}

static void* get_frame_address(uint32_t frame) {
    return (void*)(frame << 12);
}

static void flush_tlb_single(uint32_t addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static void flush_tlb_all(void) {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    asm volatile("mov %0, %%cr3" :: "r" (cr3) : "memory");
}

void paging_init(void) {
    terminal_writestring("Initializing virtual memory system...\n");
    
    static page_directory_t static_kernel_dir;
    kernel_directory = &static_kernel_dir;
    
    for (int i = 0; i < ENTRIES_PER_DIRECTORY; i++) {
        kernel_directory->tables[i].present = 0;
        kernel_directory->tables[i].writable = 1;
        kernel_directory->tables[i].user = 0;
        kernel_directory->tables[i].frame = 0;
    }
    
    for (uint32_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        if (!map_page(kernel_directory, addr, addr, PAGE_PRESENT | PAGE_WRITE)) {
            terminal_writestring("ERROR: Failed to map kernel pages\n");
            return;
        }
    }
    
    current_directory = kernel_directory;
    
    terminal_writestring("Virtual memory system initialized\n");
}

void enable_paging(void) {
    terminal_writestring("Enabling paging...\n");
    
    if (!kernel_directory) {
        terminal_writestring("ERROR: No kernel directory\n");
        return;
    }
    
    switch_page_directory(kernel_directory);
    
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
    
    terminal_writestring("Paging enabled successfully!\n");
}

page_directory_t* create_page_directory(void) {
    page_directory_t* dir = (page_directory_t*)pmm_alloc_page();
    if (!dir) {
        return NULL;
    }
    
    for (int i = 0; i < ENTRIES_PER_DIRECTORY; i++) {
        dir->tables[i].present = 0;
        dir->tables[i].writable = 1;
        dir->tables[i].user = 1;
        dir->tables[i].frame = 0;
    }
    
    for (int i = 768; i < ENTRIES_PER_DIRECTORY; i++) {
        dir->tables[i] = kernel_directory->tables[i];
    }
    
    return dir;
}

void destroy_page_directory(page_directory_t* dir) {
    if (!dir || dir == kernel_directory) {
        return;
    }
    
    for (int i = 0; i < 768; i++) {
        if (dir->tables[i].present) {
            page_table_t* table = (page_table_t*)get_frame_address(dir->tables[i].frame);
            pmm_free_page(table);
        }
    }
    
    pmm_free_page(dir);
}

void switch_page_directory(page_directory_t* dir) {
    current_directory = dir;
    uint32_t phys_addr = (uint32_t)dir;
    asm volatile("mov %0, %%cr3" :: "r"(phys_addr));
}

int map_page(page_directory_t* dir, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t page_dir_idx = virtual_addr >> 22;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;
    
    static page_table_t static_tables[4];
    static int table_count = 0;
    
    if (!dir->tables[page_dir_idx].present) {
        if (table_count >= 4) {
            return 0;
        }
        
        page_table_t* table = &static_tables[table_count++];
        
        for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
            table->pages[i].present = 0;
            table->pages[i].frame = 0;
        }
        
        dir->tables[page_dir_idx].present = 1;
        dir->tables[page_dir_idx].writable = (flags & PAGE_WRITE) ? 1 : 0;
        dir->tables[page_dir_idx].user = (flags & PAGE_USER) ? 1 : 0;
        dir->tables[page_dir_idx].frame = get_page_frame(table);
    }
    
    page_table_t* table = (page_table_t*)get_frame_address(dir->tables[page_dir_idx].frame);
    
    table->pages[page_table_idx].present = (flags & PAGE_PRESENT) ? 1 : 0;
    table->pages[page_table_idx].writable = (flags & PAGE_WRITE) ? 1 : 0;
    table->pages[page_table_idx].user = (flags & PAGE_USER) ? 1 : 0;
    table->pages[page_table_idx].frame = get_page_frame((void*)physical_addr);
    
    flush_tlb_single(virtual_addr);
    
    return 1;
}

void unmap_page(page_directory_t* dir, uint32_t virtual_addr) {
    uint32_t page_dir_idx = virtual_addr >> 22;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;
    
    if (!dir->tables[page_dir_idx].present) {
        return;
    }
    
    page_table_t* table = (page_table_t*)get_frame_address(dir->tables[page_dir_idx].frame);
    table->pages[page_table_idx].present = 0;
    table->pages[page_table_idx].frame = 0;
    
    flush_tlb_single(virtual_addr);
}

uint32_t get_physical_address(page_directory_t* dir, uint32_t virtual_addr) {
    uint32_t page_dir_idx = virtual_addr >> 22;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;
    uint32_t offset = virtual_addr & 0xFFF;
    
    if (!dir->tables[page_dir_idx].present) {
        return 0;
    }
    
    page_table_t* table = (page_table_t*)get_frame_address(dir->tables[page_dir_idx].frame);
    
    if (!table->pages[page_table_idx].present) {
        return 0;
    }
    
    return (table->pages[page_table_idx].frame << 12) + offset;
}

page_directory_t* get_kernel_directory(void) {
    return kernel_directory;
}

page_directory_t* get_current_directory(void) {
    return current_directory;
}

void page_fault_handler(void) {
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    terminal_writestring("Page fault at address: ");
    terminal_writestring("CRITICAL ERROR\n");
    
    while(1) {
        asm("hlt");
    }
}