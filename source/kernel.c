#include <stdint.h>
#include <string.h> // For memcpy, strcmp

#include "boot/multiboot.h"

#include "boot/gdt/gdt.h"
#include "boot/idt/idt.h"

#include "system/usermode/usermode.h"
#include "system/shell/shell.h"
#include "system/interrupts/interrupts.h"
#include "system/memory/physical.h" 
#include "system/filesystem/ext2/ext2.h"

#define KERNEL_HALT while (1) __asm__ volatile("hlt")

static uint8_t kernel_stack[0x2000] __attribute__((aligned(16))); // 8KB kernel stack
uint8_t* kernel_stack_top = kernel_stack + sizeof(kernel_stack);

shell_instance_t* g_kernel_shell = 0;

// Global EXT2 filesystem instance
ext2_fs_t g_ext2_fs;

extern const uint8_t _binary_disk_img_start[];
extern const uint8_t _binary_disk_img_end[];

// Callback context for directory reading
typedef struct 
{
    const char* target_name;
    uint32_t found_inode;
    bool found;
}
 find_file_ctx_t;

int test_strcmp(const char* s1, const char* s2) 
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)(*s1) - (unsigned char)(*s2);
}

// Callback function for ext2_read_dir to find a specific file by name
bool find_file_callback(const char* name, uint32_t inode, void* ctx) 
{
    find_file_ctx_t* ffc = (find_file_ctx_t*)ctx;
    // Compare the current directory entry name with the target name
    if (test_strcmp(name, ffc->target_name) == 0) {
        ffc->found_inode = inode; // Store the inode number if found
        ffc->found = true;        // Set found flag
        return false;             // Stop iterating through the directory
    }
    return true; // Continue iterating
}

bool print_dir_cb(const char* name, uint32_t inode, void* ctx) 
{
    sh_printf(g_kernel_shell,
              " - %s\r\n",
              name, inode);
    return true;  // keep iterating
}

// Helper function to find an inode by name in the root directory (simplified)
uint32_t ext2_find_file_inode_by_name(ext2_fs_t* fs, const char* filename) 
{
    ext2_inode_t root_inode;
    if (!ext2_read_inode(fs, EXT2_ROOT_INO, &root_inode)) 
    {
        sh_puts(g_kernel_shell, "EXT2: Can't read root inode\r\n");
        return 0;
    }

    // 1) Debug dump:
    sh_puts(g_kernel_shell, "Disk Contents:\r\n");
    ext2_read_dir(fs, &root_inode, print_dir_cb, NULL);

    // 2) Actual search:
    find_file_ctx_t ctx = { .target_name = filename, .found_inode = 0, .found = false };
    ext2_read_dir(fs, &root_inode, find_file_callback, &ctx);

    if (!ctx.found) {
        sh_printf(g_kernel_shell,
                  "DEBUG: '%s' not found in root (tried exact match)\r\n",
                  filename);
    }
    return ctx.found_inode;
}


void kernel_entry(uint32_t mb2_magic, uint32_t mb2_address)
{
    /* Text-Mode Shell Output */
    shell_instance_t ksh;
    sh_init(&ksh, 0xB8000, VGA_WIDTH, VGA_WIDTH * VGA_HEIGHT);
    g_kernel_shell = &ksh;

    // Initialize GDT
    if(!gdt_setup() || !gdt_verify_integrity())
    {
        sh_puts(&ksh, "FATAL: Failed to initialize GDT!\r\n");
        KERNEL_HALT;
    }

    // Initialize IDT
    if(!idt_setup() || !idt_verify_integrity())
    {
        sh_puts(&ksh, "FATAL: Failed to initialize IDT!\r\n");
        KERNEL_HALT;
    }

    // Initialize PIC (Programmable Interrupt Controller)
    pic_init();

    // Set up kernel stack in TSS (Task State Segment)
    tss_set_kernel_stack((uint32_t)kernel_stack_top);

    // Load TSS
    __asm__ volatile ("ltr %%ax" : : "a" (GDT_TSS_SEL));

    // Enable interrupts
    __asm__ volatile("sti");

    // Get total memory from Multiboot2 info
    uint64_t total_memory_bytes = mb2_get_memory(mb2_magic, mb2_address);
    sh_printf(&ksh, "Booted with %d MB of Memory.\r\n", (int)(total_memory_bytes / (1024 * 1024)));

    // Initialize physical memory manager
    // We assume usable memory starts after 1MB (0x100000) to avoid kernel code/data.
    // In a real system, you'd parse the multiboot memory map for exact regions.
    void* pmm_start_addr = (void*)0x100000;
    size_t pmm_total_size = total_memory_bytes - (size_t)pmm_start_addr;
    mem_phys_init(pmm_start_addr, pmm_total_size);

    // Use the embedded disk image directly
    if (ext2_mount(&g_ext2_fs, (uint8_t*)_binary_disk_img_start)) 
    {
        sh_printf(&ksh, "EXT2 filesystem mounted successfully! Block size: %d bytes\r\n",
                 (int)g_ext2_fs.block_size);

        // Set up user mode environment with specific program
        um_setup_env("example.bin");
        
        // Switch to user mode
        um_switch();
    } 
    else 
    {
        sh_puts(&ksh, "Failed to mount EXT2 filesystem from embedded image.\r\n");
    }

    // Main kernel loop - make sure we're actively rendering
    while(1) 
    {
        __asm__ volatile("hlt");
    }

    // Should never reach here if user mode runs correctly
    sh_puts(&ksh, "Returned from user mode unexpectedly!\r\n");
    KERNEL_HALT;
}