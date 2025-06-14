# ─── Directories ───────────────────────────────────────────────────────
SRC_DIR      := source
BUILD_DIR    := build
ISO_DIR      := iso
BOOT_DIR     := $(ISO_DIR)/boot
GRUB_DIR     := $(BOOT_DIR)/grub
ROOTFS_DIR   := ./rootfs

# ─── Disk Image ────────────────────────────────────────────────────────
DISK_IMG     := disk.img
DISK_INC_SRC := $(BUILD_DIR)/disk-img.S
DISK_OBJ     := $(BUILD_DIR)/disk-img.o

# ─── Toolchain ─────────────────────────────────────────────────────────
CC           := gcc
LD           := ld
AS           := nasm
CFLAGS       := -m32 -ffreestanding -Wall -Wextra
ASFLAGS      := -f elf32
LDFLAGS      := -m elf_i386 -T linker.ld

# ─── Sources & Objects ─────────────────────────────────────────────────
C_SRCS       := $(shell find $(SRC_DIR) -type f -name '*.c')
ASM_SRCS     := $(shell find $(SRC_DIR) -type f -name '*.asm')
C_OBJS       := $(patsubst $(SRC_DIR)/%.c,   $(BUILD_DIR)/%.o, $(C_SRCS))
ASM_OBJS     := $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))
OBJ_FILES    := $(C_OBJS) $(ASM_OBJS) $(DISK_OBJ)

# ─── Default Target ────────────────────────────────────────────────────
.PHONY: all clean run
all: kernel.iso

# ─── Compile C ────────────────────────────────────────────────────────
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ─── Assemble ASM ─────────────────────────────────────────────────────
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# ─── Generate ext2 disk image from rootfs (no sudo, simpler) ─────────
$(DISK_IMG): $(shell find $(ROOTFS_DIR) -type f)
	@echo ">>> Creating ext2 disk image from $(ROOTFS_DIR)"
	genext2fs -b 20480 -d $(ROOTFS_DIR) $@
	@echo ">>> Disk image '$@' created."

# ─── Generate NASM source for embedding disk image ─────────────────────
$(DISK_INC_SRC): $(DISK_IMG)
	@mkdir -p $(dir $@)
	@echo "; autogenerated: embed $(DISK_IMG) into .binary_disk_img"    > $@
	@echo "section .binary_disk_img"                                   >> $@
	@echo "  global _binary_disk_img_start, _binary_disk_img_end, _binary_disk_img_size" >> $@
	@echo "_binary_disk_img_start:"                                     >> $@
	@echo "  incbin \"$(abspath $(DISK_IMG))\""                        >> $@
	@echo "_binary_disk_img_end:"                                       >> $@
	@echo "_binary_disk_img_size: equ _binary_disk_img_end - _binary_disk_img_start" >> $@

# ─── Assemble incbin glue ─────────────────────────────────────────────
$(DISK_OBJ): $(DISK_INC_SRC)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# ─── Link everything into kernel ELF ───────────────────────────────────
build/kernel.elf: $(OBJ_FILES)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

# ─── Make bootable ISO with GRUB ──────────────────────────────────────
kernel.iso: build/kernel.elf grub/grub.cfg
	@mkdir -p $(GRUB_DIR)
	cp build/kernel.elf $(BOOT_DIR)/
	cp grub/grub.cfg    $(GRUB_DIR)/
	grub-mkrescue -o $@ $(ISO_DIR)

# ─── Run in QEMU ──────────────────────────────────────────────────────
run: kernel.iso
	qemu-system-i386 -cdrom kernel.iso

# ─── Clean up ─────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) kernel.iso $(DISK_IMG)
