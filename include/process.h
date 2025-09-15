#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"

#define MAX_PROCESSES 32
#define PROCESS_STACK_SIZE 0x2000
#define USER_STACK_TOP 0xBFFFF000

typedef enum {
    PROCESS_CREATED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, esp, ebp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} cpu_state_t;

typedef struct process {
    uint32_t pid;
    process_state_t state;
    cpu_state_t cpu_state;
    page_directory_t* page_directory;
    void* stack_base;
    size_t stack_size;
    uint32_t entry_point;
    struct process* next;
} process_t;

void process_init(void);
process_t* create_process(void* elf_data, size_t elf_size);
void destroy_process(process_t* process);
void schedule(void);
process_t* get_current_process(void);
void yield(void);

void context_switch(process_t* old_process, process_t* new_process);
int run_process(process_t* process);

#endif