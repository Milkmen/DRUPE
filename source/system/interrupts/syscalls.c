#include "syscalls.h"

#include "../shell/shell.h"

#include <stdint.h>
#include <stddef.h>

extern shell_instance_t* g_kernel_shell;

void exit_to_kernel(void) {
    extern shell_instance_t* g_kernel_shell;
    if (g_kernel_shell) 
    {
        sh_puts(g_kernel_shell, "Idling...\r\n");
    }

    // Kernel idle loop
    while (1) 
    {
        __asm__ volatile("hlt");
    }
}

void handle_syscall(interrupt_frame_t* frame)
{
    extern uint8_t* kernel_stack_top;
    uint32_t syscall_num = frame->eax;
    uint32_t arg0 = frame->ebx; // stream id
    uint32_t arg1 = frame->ecx; // buffer
    uint32_t arg2 = frame->edx; // length
    
    // Check if call came from user mode
    if ((frame->cs & 0x3) != 3) {
        sh_puts(g_kernel_shell, "System call from kernel mode - ignoring\r\n");
        return;
    }

    switch (syscall_num) 
    {
        case SYS_WRITE:
            {
                const char* str = (const char*)arg1;
                if (g_kernel_shell && str) 
                {
                    // More permissive range check for user memory
                    if (arg1 >= 0x400000 && arg1 < 0x900000) 
                    {
                        sh_write_stream(g_kernel_shell, arg0, str, arg2);

                        frame->eax = arg2; // Return number of bytes written
                    }
                    else 
                    {
                        sh_printf(g_kernel_shell, "Invalid string pointer: 0x%x\r\n", arg1);
                        frame->eax = -1; // Error
                    }
                } 
                else 
                {
                    frame->eax = -1; // Error
                }
            }
            break;
            
        case SYS_EXIT:
            {
                if (g_kernel_shell) 
                {
                    sh_printf(g_kernel_shell, "Exited with code %d\r\n", arg0);
                }
                
                frame->eax = 0;
                
                frame->cs = 0x08;  // Kernel code segment
                frame->ss = 0x10;  // Kernel data segment
                frame->eip = (uint32_t)exit_to_kernel;  // Jump to kernel exit handler
                frame->useresp = (uint32_t)kernel_stack_top;  // Use kernel stack
                    
                // Clear user mode segments
                frame->ds = 0x10;
                frame->es = 0x10;
                frame->fs = 0x10;
                frame->gs = 0x10;
            }
            break;
            
        case SYS_GETPID:
            {
                frame->eax = 1; // Process ID 1
            }
            break;
            
        case SYS_READ:
            {
                char* buf = (char*)arg1;
                size_t count = arg2;
                
                if (g_kernel_shell && buf) 
                {
                    // Check buffer is in user memory
                    if (arg1 >= 0x400000 && arg1 < 0x900000) 
                    {
                        // Read from stdin stream
                        frame->eax = stream_read(&g_kernel_shell->streams[STREAM_STDIN], buf, count);
                    }
                    else 
                    {
                        sh_printf(g_kernel_shell, "Invalid buffer pointer: 0x%x\r\n", arg1);
                        frame->eax = -1;
                    }
                } 
                else 
                {
                    frame->eax = -1;
                }
            }
            break;
            
        default:
            if (g_kernel_shell) {
                sh_printf(g_kernel_shell, "Unknown system call: %d\r\n", syscall_num);
            }
            frame->eax = -1; // Error
            break;
    }
}