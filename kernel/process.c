#include "../include/process.h"
#include "../include/memory.h"
#include "../include/paging.h"
#include "../include/elf.h"
#include <stddef.h>

extern void terminal_writestring(const char* data);
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static process_t* process_list = NULL;
static process_t* current_process = NULL;
static uint32_t next_pid = 1;

void process_init(void) {
    terminal_writestring("Initializing process management...\n");
    
    process_list = NULL;
    current_process = NULL;
    next_pid = 1;
    
    terminal_writestring("Process management initialized\n");
}

static void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static void* memset(void* s, int c, size_t n) {
    char* p = (char*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = c;
    }
    return s;
}

process_t* create_process(void* elf_data, size_t elf_size) {
    terminal_writestring("Creating new process...\n");
    
    if (!elf_validate(elf_data)) {
        terminal_writestring("ERROR: Invalid ELF file\n");
        return NULL;
    }
    
    process_t* process = (process_t*)kmalloc(sizeof(process_t));
    if (!process) {
        terminal_writestring("ERROR: Failed to allocate process structure\n");
        return NULL;
    }
    
    process->pid = next_pid++;
    process->state = PROCESS_CREATED;
    process->page_directory = NULL;
    
    void* entry_point = elf_get_entry_point(elf_data);
    if (!entry_point) {
        terminal_writestring("ERROR: Invalid entry point\n");
        kfree(process);
        return NULL;
    }
    
    process->entry_point = (uint32_t)entry_point;
    
    if (!elf_load(elf_data, elf_size)) {
        terminal_writestring("ERROR: Failed to load ELF\n");
        kfree(process);
        return NULL;
    }
    
    process->stack_base = (void*)USER_STACK_TOP;
    process->stack_size = PROCESS_STACK_SIZE;
    
    process->cpu_state.esp = USER_STACK_TOP - 16;
    process->cpu_state.ebp = USER_STACK_TOP - 16;
    process->cpu_state.eip = process->entry_point;
    process->cpu_state.eflags = 0x202;
    process->cpu_state.cs = 0x08;
    process->cpu_state.ds = process->cpu_state.es = 
    process->cpu_state.fs = process->cpu_state.gs = 
    process->cpu_state.ss = 0x10;
    
    process->state = PROCESS_READY;
    
    process->next = process_list;
    process_list = process;
    
    terminal_writestring("Process created successfully\n");
    return process;
}

void destroy_process(process_t* process) {
    if (!process) return;
    
    if (process == current_process) {
        current_process = NULL;
    }
    
    process_t** current = &process_list;
    while (*current) {
        if (*current == process) {
            *current = process->next;
            break;
        }
        current = &(*current)->next;
    }
    
    kfree(process);
}

void schedule(void) {
    if (!process_list) {
        return;
    }
    
    if (!current_process) {
        current_process = process_list;
    } else {
        current_process = current_process->next;
        if (!current_process) {
            current_process = process_list;
        }
    }
    
    if (current_process && current_process->state == PROCESS_READY) {
        current_process->state = PROCESS_RUNNING;
    }
}

process_t* get_current_process(void) {
    return current_process;
}

void yield(void) {
    if (current_process) {
        current_process->state = PROCESS_READY;
    }
    schedule();
}

int run_process(process_t* process) {
    if (!process || process->state != PROCESS_READY) {
        return -1;
    }
    
    terminal_writestring("Running process (identity mapping mode)...\n");
    
    current_process = process;
    process->state = PROCESS_RUNNING;
    
    typedef int (*process_entry_t)(void);
    process_entry_t entry = (process_entry_t)process->entry_point;
    
    int result = entry();
    
    process->state = PROCESS_TERMINATED;
    current_process = NULL;
    
    terminal_writestring("Process completed successfully\n");
    
    return result;
}