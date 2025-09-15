#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define ENTRIES_PER_TABLE 1024
#define ENTRIES_PER_DIRECTORY 1024

#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40

#define VIRTUAL_BASE    0xC0000000
#define KERNEL_BASE     0xC0000000
#define USER_BASE       0x40000000

typedef struct {
    uint32_t present    : 1;
    uint32_t writable   : 1; 
    uint32_t user       : 1;
    uint32_t writethrough : 1;
    uint32_t cachedisable : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t size       : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame      : 20;
} __attribute__((packed)) page_t;

typedef struct {
    page_t pages[ENTRIES_PER_TABLE];
} page_table_t;

typedef struct {
    uint32_t present    : 1;
    uint32_t writable   : 1;
    uint32_t user       : 1;
    uint32_t writethrough : 1;
    uint32_t cachedisable : 1;
    uint32_t accessed   : 1;
    uint32_t reserved   : 1;
    uint32_t size       : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame      : 20;
} __attribute__((packed)) page_directory_entry_t;

typedef struct {
    page_directory_entry_t tables[ENTRIES_PER_DIRECTORY];
} page_directory_t;

void paging_init(void);
void enable_paging(void);
page_directory_t* create_page_directory(void);
void destroy_page_directory(page_directory_t* dir);
void switch_page_directory(page_directory_t* dir);

int map_page(page_directory_t* dir, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(page_directory_t* dir, uint32_t virtual_addr);
uint32_t get_physical_address(page_directory_t* dir, uint32_t virtual_addr);

page_directory_t* get_kernel_directory(void);
page_directory_t* get_current_directory(void);

void page_fault_handler(void);

#endif