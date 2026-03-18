#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

typedef struct process {
    uint32_t id;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t stack_base;
    uint32_t state;
    struct process* next;
} process_t;

typedef struct {
    uint32_t id;
    uint32_t state;
    bool     current;
} scheduler_proc_info_t;

void scheduler_init(void);
int create_process(void (*entry_point)());
void yield(void);
int scheduler_snapshot(scheduler_proc_info_t *out, int max_count);
uint32_t scheduler_process_count(void);
uint32_t scheduler_current_pid(void);
uint32_t scheduler_tick_count(void);
bool scheduler_kill(uint32_t pid);

#endif
