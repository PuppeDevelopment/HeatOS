#include "scheduler.h"
#include "mm.h"
#include "klog.h"
#include "panic.h"

static process_t* current_process = 0;
static process_t* process_list = 0;
static uint32_t next_pid = 1;
static uint32_t scheduler_ticks = 0;

static uint32_t scheduler_count_internal(void) {
    if (!process_list) return 0;

    uint32_t count = 1;
    process_t *p = process_list->next;
    while (p && p != process_list) {
        count++;
        p = p->next;
        if (count > 4096) break;
    }
    return count;
}

void scheduler_init(void) {
    // Current kernel execution becomes PID 0
    process_t* kernel_proc = (process_t*)kmalloc(sizeof(process_t));
    if (!kernel_proc) {
        PANIC("scheduler init failed: kmalloc returned null");
    }

    kernel_proc->id = 0;
    kernel_proc->esp = 0;
    kernel_proc->ebp = 0;
    kernel_proc->eip = 0;
    kernel_proc->stack_base = 0;
    kernel_proc->state = 1;
    kernel_proc->next = kernel_proc;
    
    current_process = kernel_proc;
    process_list = kernel_proc;
}

int create_process(void (*entry_point)()) {
    process_t* new_proc = (process_t*)kmalloc(sizeof(process_t));
    if (!new_proc || !process_list) {
        klog_error("sched", "create_process failed: no process struct memory");
        return -1;
    }

    new_proc->id = next_pid++;
    new_proc->state = 1;
    
    // Allocate stack
    void* stack = pmm_alloc_page();
    if (!stack) {
        klog_error("sched", "create_process failed: no free page for stack");
        return -1;
    }

    uint32_t* stack_top = (uint32_t*)((uint32_t)stack + 4096);
    
    // Fake context for first switch
    *(--stack_top) = (uint32_t)entry_point; // EIP
    *(--stack_top) = 0; // EBP
    *(--stack_top) = 0; // ESP
    *(--stack_top) = 0; // EBX
    *(--stack_top) = 0; // ESI
    *(--stack_top) = 0; // EDI
    
    new_proc->esp = (uint32_t)stack_top;
    new_proc->stack_base = (uint32_t)stack;
    
    // Insert into ring
    new_proc->next = process_list->next;
    process_list->next = new_proc;

    klog_log_u32(KLOG_INFO, "sched", "created pid", new_proc->id, 10);
    
    return new_proc->id;
}

void yield(void) {
    if (!current_process || current_process == current_process->next) return;
    
    // Minimal round-robin queue tick
    current_process = current_process->next;
    scheduler_ticks++;
    
    // In a real OS we'd invoke the context switch ASM routine here
}

int scheduler_snapshot(scheduler_proc_info_t *out, int max_count) {
    if (!out || max_count <= 0 || !process_list) return 0;

    int written = 0;
    process_t *p = process_list;
    do {
        if (written >= max_count) break;
        out[written].id = p->id;
        out[written].state = p->state;
        out[written].current = (p == current_process);
        written++;
        p = p->next;
    } while (p && p != process_list);

    return written;
}

uint32_t scheduler_process_count(void) {
    return scheduler_count_internal();
}

uint32_t scheduler_current_pid(void) {
    if (!current_process) return 0;
    return current_process->id;
}

uint32_t scheduler_tick_count(void) {
    return scheduler_ticks;
}

bool scheduler_kill(uint32_t pid) {
    if (!process_list) return false;
    if (pid == 0) return false; // never kill kernel process

    process_t *prev = process_list;
    process_t *cur = process_list->next;

    while (cur && cur != process_list) {
        if (cur->id == pid) {
            prev->next = cur->next;
            if (current_process == cur) {
                current_process = cur->next ? cur->next : process_list;
            }

            if (cur->stack_base) {
                pmm_free_page((void*)cur->stack_base);
            }

            kfree(cur);
            klog_log_u32(KLOG_WARN, "sched", "killed pid", pid, 10);
            return true;
        }
        prev = cur;
        cur = cur->next;
    }

    return false;
}
