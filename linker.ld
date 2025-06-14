ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot_header ALIGN(8) : {
        KEEP(*(.multiboot_header))
    }

    .text ALIGN(4K) : {
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(.bss)
        *(COMMON)
    }

    .disk ALIGN(4K) : {
        _binary_disk_img_start = .;
        KEEP(*(.binary_disk_img))
        _binary_disk_img_end = .;
    }
}
