#include "../include/syscall.h"
#include "../include/interrupts.h"
#include "../include/memory.h"
#include "../include/process.h"
#include "../include/keyboard.h"
#include "../include/scheduler.h"

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

void syscall_init(void) {
    register_interrupt_handler(0x80, syscall_handler);
    terminal_writestring("System calls initialized\n");
}

void syscall_handler(registers_t regs) {
    uint32_t syscall_num = regs.eax;
    uint32_t arg1 = regs.ebx;
    // uint32_t arg2 = regs.ecx;
    // uint32_t arg3 = regs.edx;
    uint32_t result = 0;
    
    switch (syscall_num) {
        case SYSCALL_PRINT:
            result = syscall_print((const char*)arg1);
            break;
            
        case SYSCALL_READ:
            result = (uint32_t)syscall_read();
            break;
            
        case SYSCALL_MALLOC:
            result = (uint32_t)syscall_malloc(arg1);
            break;
            
        case SYSCALL_FREE:
            syscall_free((void*)arg1);
            result = 0;
            break;
            
        case SYSCALL_EXIT:
            syscall_exit(arg1);
            result = 0;
            break;
            
        case SYSCALL_GETPID:
            result = syscall_getpid();
            break;
            
        case SYSCALL_YIELD:
            syscall_yield();
            result = 0;
            break;
            
        default:
            result = -1;
            break;
    }
    
    regs.eax = result;
}

int syscall_print(const char* message) {
    if (!message) {
        return -1;
    }
    
    int count = 0;
    while (message[count] && count < 1024) {
        terminal_putchar(message[count]);
        count++;
    }
    
    return count;
}

char syscall_read(void) {
    while (!keyboard_available()) {
        asm("hlt");
    }
    return keyboard_getchar();
}

void* syscall_malloc(size_t size) {
    if (size == 0 || size > 1024 * 1024) {
        return NULL;
    }
    return kmalloc(size);
}

void syscall_free(void* ptr) {
    if (ptr) {
        kfree(ptr);
    }
}

void syscall_exit(int code) {
    (void)code;
    process_t* current = get_current_process();
    if (current) {
        current->state = PROCESS_TERMINATED;
    }
}

uint32_t syscall_getpid(void) {
    process_t* current = get_current_process();
    if (current) {
        return current->pid;
    }
    return 0;
}

void syscall_yield(void) {
    yield_cpu();
}