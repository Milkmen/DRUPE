#ifndef K_INTERRUPTS_H
#define K_INTERRUPTS_H

#include <stdint.h>

typedef struct
{
    uint32_t gs, fs, es, ds;                    // Segment registers
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // General registers (pushad)
    uint32_t int_no, err_code;                  // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;      // Pushed by CPU
} interrupt_frame_t;

void pic_init(void);

#endif