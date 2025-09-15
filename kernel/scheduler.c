#include "../include/scheduler.h"
#include "../include/process.h"
#include "../include/interrupts.h"

extern void terminal_writestring(const char* data);

static process_t* ready_queue = NULL;
static process_t* current_process = NULL;
static uint32_t time_slice_counter = 0;
static uint32_t scheduler_enabled = 0;

static void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

void scheduler_init(void) {
    terminal_writestring("Initializing scheduler...\n");
    
    register_interrupt_handler(32, timer_handler);
    
    uint32_t divisor = 1193180 / TIMER_FREQUENCY;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    
    ready_queue = NULL;
    current_process = NULL;
    time_slice_counter = 0;
    scheduler_enabled = 1;
    
    terminal_writestring("Scheduler initialized with preemptive multitasking\n");
}

void timer_handler(registers_t regs) {
    (void)regs;
    
    if (!scheduler_enabled) {
        return;
    }
    
    time_slice_counter++;
    
    if (time_slice_counter >= 10) {
        time_slice_counter = 0;
        schedule_next();
    }
}

void schedule_next(void) {
    if (!scheduler_enabled || !ready_queue) {
        return;
    }
    
    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
        add_process_to_queue(current_process);
    }
    
    current_process = get_next_process();
    
    if (current_process) {
        current_process->state = PROCESS_RUNNING;
        remove_process_from_queue(current_process);
    }
}

void add_process_to_queue(process_t* process) {
    if (!process || process->state == PROCESS_TERMINATED) {
        return;
    }
    
    process->next = ready_queue;
    ready_queue = process;
}

void remove_process_from_queue(process_t* process) {
    if (!process || !ready_queue) {
        return;
    }
    
    if (ready_queue == process) {
        ready_queue = ready_queue->next;
        process->next = NULL;
        return;
    }
    
    process_t* current = ready_queue;
    while (current->next) {
        if (current->next == process) {
            current->next = process->next;
            process->next = NULL;
            return;
        }
        current = current->next;
    }
}

process_t* get_next_process(void) {
    if (!ready_queue) {
        return NULL;
    }
    
    process_t* next = ready_queue;
    ready_queue = ready_queue->next;
    next->next = NULL;
    
    return next;
}

void yield_cpu(void) {
    if (scheduler_enabled) {
        schedule_next();
    }
}

process_t* get_current_running_process(void) {
    return current_process;
}

void enable_scheduler(void) {
    scheduler_enabled = 1;
}

void disable_scheduler(void) {
    scheduler_enabled = 0;
}