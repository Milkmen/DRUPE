#include "processes.h"
#include "usermode.h"

static process_t process_table[PROC_MAX_COUNT];

static process_t* current_process = 0;

void proc_mgr_init()
{
    for(int pid = 0; pid < PROC_MAX_COUNT; ++pid)
    {
        process_table[pid].id = pid;
        process_table[pid].state = PROC_UNUSED;
        int r = 0;
        while(r < 16)
        {
            process_table[pid].regs[r] = 0x0; r++;
            process_table[pid].regs[r] = 0x0; r++;
            process_table[pid].regs[r] = 0x0; r++;
            process_table[pid].regs[r] = 0x0; r++;
        }
        process_table[pid].kernel_stack_top = 0x0;
        process_table[pid].kernel_stack_top = 0x0;
    }
}

process_t* proc_create(void)
{
    // Find free process slot
    for(int pid = 0; pid < PROC_MAX_COUNT; ++pid) 
    {
        if(process_table[pid].state == PROC_UNUSED) 
        {
            process_table[pid].state = PROC_RUNNING;
            process_table[pid].kernel_stack_top = (uintptr_t)mem_phys_alloc();
            process_table[pid].user_stack_top = USER_STACK_TOP;
            current_process = &process_table[pid];
            return current_process;
        }
    }
    return 0;
}

process_t* proc_current(void)
{
    return current_process;
}