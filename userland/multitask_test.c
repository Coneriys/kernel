// Simple test program to demonstrate multitasking
// This will be compiled to demonstrate multiple processes running

volatile int counter = 0;

void _start() {
    // Use syscalls to print and count
    for (int i = 0; i < 5; i++) {
        // Syscall 1 (write) - show we're running
        asm volatile (
            "movl $1, %%eax\n\t"     // Syscall number 1 (write)
            "movl %0, %%ebx\n\t"     // Counter value 
            "int $0x80"
            :
            : "r" (i)
            : "eax", "ebx"
        );
        
        // Small delay loop
        for (volatile int j = 0; j < 100000; j++) {
            counter++;
        }
        
        // Yield CPU to other processes
        asm volatile (
            "movl $3, %%eax\n\t"     // Syscall number 3 (yield)
            "int $0x80"
            :
            :
            : "eax"
        );
    }
    
    // Exit syscall
    asm volatile (
        "movl $2, %%eax\n\t"         // Syscall number 2 (exit)
        "movl $0, %%ebx\n\t"         // Exit code 0
        "int $0x80"
        :
        :
        : "eax", "ebx"
    );
}