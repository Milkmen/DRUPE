#include "usermode.h"
#include "processes.h"
#include "../shell/shell.h"
#include "../filesystem/ext2/ext2.h"

extern ext2_fs_t g_ext2_fs;
extern shell_instance_t* g_kernel_shell;

void um_setup_env(const char* program_name) 
{
    // Create new process
    process_t* proc = proc_create();
    if (!proc) 
    {
        sh_puts(g_kernel_shell, "Failed to create process\r\n");
        return;
    }

    // Set up TSS with kernel stack for when we return from user mode
    tss_set_kernel_stack(proc->kernel_stack_top);
    
    // Find program file
    ext2_inode_t inode;
    uint32_t inode_num = ext2_find_file_inode_by_name(&g_ext2_fs, program_name);
    if (!inode_num || !ext2_read_inode(&g_ext2_fs, inode_num, &inode)) 
    {
        sh_printf(g_kernel_shell, "Failed to find program: %s\r\n", program_name);
        return;
    }

    // Read program into user space
    uint8_t* user_code = (uint8_t*)USER_CODE_BASE;
    size_t bytes_read = ext2_read_file(&g_ext2_fs, &inode, user_code, inode.i_size_lo, 0);
    
    if (bytes_read != inode.i_size_lo) 
    {
        sh_printf(g_kernel_shell, "Failed to read complete program (read %d of %d bytes)\r\n", 
                 bytes_read, inode.i_size_lo);
        return;
    }
}

void um_switch(void) 
{
    // Set up user data segment
    __asm__ volatile (
        "mov $0x23, %%ax\n\t"      // User data selector (GDT entry 4, RPL=3)
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        :
        :
        : "ax"
    );
    
    // Push values for iret to user mode
    __asm__ volatile (
        "pushl $0x23\n\t"              // SS (user data selector)
        "pushl %0\n\t"                 // ESP (user stack)
        "pushf\n\t"                    // EFLAGS
        "orl $0x200, (%%esp)\n\t"      // Enable interrupts in EFLAGS
        "pushl $0x1B\n\t"              // CS (user code selector)
        "pushl %1\n\t"                 // EIP (user program address)
        "iret\n\t"                     // Switch to user mode
        :
        : "r" (USER_STACK_TOP), "r" (USER_CODE_BASE)
        : "memory"
    );
}