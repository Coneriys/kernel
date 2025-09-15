#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include "interrupts.h"

#define SYSCALL_PRINT    0
#define SYSCALL_READ     1
#define SYSCALL_MALLOC   2
#define SYSCALL_FREE     3
#define SYSCALL_EXIT     4
#define SYSCALL_GETPID   5
#define SYSCALL_YIELD    6

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
} syscall_args_t;

void syscall_init(void);
void syscall_handler(registers_t regs);

int syscall_print(const char* message);
char syscall_read(void);
void* syscall_malloc(size_t size);
void syscall_free(void* ptr);
void syscall_exit(int code);
uint32_t syscall_getpid(void);
void syscall_yield(void);

static inline int sys_print(const char* message) {
    int result;
    asm volatile (
        "mov $0, %%eax\n"
        "mov %1, %%ebx\n"
        "int $0x80\n"
        "mov %%eax, %0"
        : "=r" (result)
        : "r" (message)
        : "eax", "ebx"
    );
    return result;
}

static inline char sys_read(void) {
    char result;
    asm volatile (
        "mov $1, %%eax\n"
        "int $0x80\n"
        "mov %%al, %0"
        : "=r" (result)
        :
        : "eax"
    );
    return result;
}

static inline void* sys_malloc(size_t size) {
    void* result;
    asm volatile (
        "mov $2, %%eax\n"
        "mov %1, %%ebx\n"
        "int $0x80\n"
        "mov %%eax, %0"
        : "=r" (result)
        : "r" (size)
        : "eax", "ebx"
    );
    return result;
}

static inline void sys_free(void* ptr) {
    asm volatile (
        "mov $3, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80"
        :
        : "r" (ptr)
        : "eax", "ebx"
    );
}

static inline void sys_exit(int code) {
    asm volatile (
        "mov $4, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80"
        :
        : "r" (code)
        : "eax", "ebx"
    );
}

static inline uint32_t sys_getpid(void) {
    uint32_t result;
    asm volatile (
        "mov $5, %%eax\n"
        "int $0x80\n"
        "mov %%eax, %0"
        : "=r" (result)
        :
        : "eax"
    );
    return result;
}

static inline void sys_yield(void) {
    asm volatile (
        "mov $6, %%eax\n"
        "int $0x80"
        :
        :
        : "eax"
    );
}

#endif