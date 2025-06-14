#ifndef K_SYSCALLS_H
#define K_SYSCALLS_H

#include "interrupts.h"

#define SYS_EXIT    0x01
#define SYS_WRITE   0x04
#define SYS_GETPID  0x14
#define SYS_READ    0x03

void handle_syscall(interrupt_frame_t* frame);

#endif