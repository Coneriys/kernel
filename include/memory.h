#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

typedef struct multiboot_memory_map {
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_t;

void pmm_init(multiboot_info_t* mbi);
void* pmm_alloc_page(void);
void pmm_free_page(void* page);
size_t pmm_get_total_memory(void);
size_t pmm_get_free_memory(void);

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif