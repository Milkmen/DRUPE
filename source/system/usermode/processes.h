#ifndef K_PROC_MGR_H
#define K_PROC_MGR_H

#include <stdint.h>

#define PROC_MAX_COUNT  64

#define PROC_UNUSED     0
#define PROC_RUNNING    1
#define PROC_PAUSED     2

typedef struct
{
    uint16_t id;
    uint8_t state;
    uintptr_t kernel_stack_top;
    uintptr_t user_stack_top;
    uint64_t regs[16];
}
process_t;

#endif