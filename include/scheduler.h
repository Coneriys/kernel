#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h"
#include "interrupts.h"

#define TIMER_FREQUENCY 100

void scheduler_init(void);
void timer_handler(registers_t regs);
void schedule_next(void);
void add_process_to_queue(process_t* process);
void remove_process_from_queue(process_t* process);

process_t* get_next_process(void);
void yield_cpu(void);
process_t* get_current_running_process(void);
void enable_scheduler(void);
void disable_scheduler(void);

#endif