// Inline syscall functions for userland programs
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

static inline void* sys_malloc(int size) {
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

static inline int sys_getpid(void) {
    int result;
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

int main(void) {
    sys_print("Hello from userland with syscalls!\n");
    
    int pid = sys_getpid();
    sys_print("My process ID is: ");
    
    void* memory = sys_malloc(1024);
    if (memory) {
        sys_print("Successfully allocated 1024 bytes!\n");
        sys_free(memory);
        sys_print("Memory freed successfully!\n");
    } else {
        sys_print("Failed to allocate memory!\n");
    }
    
    sys_print("Syscall test completed!\n");
    
    return 42;
}