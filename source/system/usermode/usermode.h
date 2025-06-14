#ifndef K_USERMODE_H
#define K_USERMODE_H

#include <stdint.h>
#include "../../boot/gdt/gdt.h"

#define USER_STACK_SIZE     0x2000      // 8KB user stack
#define USER_STACK_TOP      0x800000    // 8MB mark for user stack
#define USER_CODE_BASE      0x400000    // 4MB mark for user code

void um_switch(void);

void um_setup_env(const char* program_name);

#endif