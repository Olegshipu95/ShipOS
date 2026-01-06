# ==============================
# Toolchain
# ==============================
NASM := nasm
NASM_FLAGS := -f elf64           # Output 64-bit ELF objects for assembly

CC := gcc
CFLAGS := -Wall -c -ggdb -ffreestanding -mgeneral-regs-only  # Compile C for bare metal

LD := ld
LINKER := x86_64/boot/linker.ld
LD_FLAGS := --nmagic --script=$(LINKER)  # Use custom linker script

GRUB := grub-mkrescue
GRUB_FLAGS := -o

QEMU := qemu-system-x86_64
QEMU_FLAGS := -m 128M -cdrom
QEMU_HEADLESS_FLAGS := -nographic -serial file:serial.log

# ==============================
# Directories
# ==============================
ISO_DIR := isofiles
ISO_BOOT_DIR := $(ISO_DIR)/boot
BUILD_DIR := build

# ==============================
# VFS Configuration
# ==============================
VFS_CONFIG := configs/fs.conf
VFS_CONFIG_GEN := $(BUILD_DIR)/kernel/vfs/configs/vfs_config.c
VFS_CONFIG_H := kernel/vfs/configs/vfs_config.h
VFS_GEN_SCRIPT := scripts/kernel/generate_vfs_config.sh

# ==============================
# Source files
# ==============================
# All 64-bit assembly in x86_64/
x86_64_asm_sources := $(shell find x86_64 -name '*.asm')
x86_64_asm_objects := $(patsubst x86_64/%.asm,$(BUILD_DIR)/x86_64/%.o,$(x86_64_asm_sources))

# All C files in kernel/ (excluding generated vfs_config.c to avoid duplicates)
kernel_c_sources := $(shell find kernel -name '*.c' ! -path '*/vfs/vfs_config.c')
kernel_c_objects := $(patsubst kernel/%.c,$(BUILD_DIR)/kernel/%.o,$(kernel_c_sources))
# Add generated vfs_config object (handled by special rule below)
kernel_c_objects += $(BUILD_DIR)/kernel/vfs/vfs_config.o

# All assembly files in kernel/
kernel_asm_sources := $(shell find kernel -name '*.asm')
kernel_asm_objects := $(patsubst kernel/%.asm,$(BUILD_DIR)/kernel/%.o,$(kernel_asm_sources))

# All object files
objects := $(x86_64_asm_objects) $(kernel_c_objects) $(kernel_asm_objects)

# ==============================
# Build rules
# ==============================

# Generate VFS configuration C file from config file
$(VFS_CONFIG_GEN): $(VFS_CONFIG) $(VFS_GEN_SCRIPT)
	@mkdir -p $(dir $@)
	@$(VFS_GEN_SCRIPT) $(VFS_CONFIG) $@

# Compile generated VFS config file
$(BUILD_DIR)/kernel/vfs/vfs_config.o: $(VFS_CONFIG_GEN) $(VFS_CONFIG_H)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. $(VFS_CONFIG_GEN) -o $@

# Compile assembly files
$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_FLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# Link all object files into kernel binary
$(ISO_BOOT_DIR)/kernel.bin: $(objects) | $(VFS_CONFIG_GEN)
	@mkdir -p $(ISO_BOOT_DIR)
	$(LD) $(LD_FLAGS) --output=$@ $^

# Create bootable ISO using GRUB
$(ISO_DIR)/kernel.iso: $(ISO_BOOT_DIR)/kernel.bin
	$(GRUB) $(GRUB_FLAGS) $@ $(ISO_DIR)

# ==============================
# Phony targets
# ==============================
.PHONY: build_kernel build_iso qemu qemu-headless clean install

# Build kernel binary only
build_kernel: $(ISO_BOOT_DIR)/kernel.bin

# Build ISO
build_iso: $(ISO_DIR)/kernel.iso

# Run QEMU in BIOS mode
qemu: $(ISO_DIR)/kernel.iso
	$(QEMU) $(QEMU_FLAGS) $(ISO_DIR)/kernel.iso

# Run QEMU in Headless mode (no GUI, logs to serial.log)
qemu-headless: $(ISO_DIR)/kernel.iso
	@echo "Running in headless mode. Logs will be saved to serial.log"
	@echo "Press Ctrl+C to stop QEMU"
	$(QEMU) $(QEMU_HEADLESS_FLAGS) $(QEMU_FLAGS) $(ISO_DIR)/kernel.iso

# ==============================
# GDB integration
# ==============================
# Generate unique GDB port per user
GDBPORT := $(shell expr `id -u` % 5000 + 25000)

# Determine QEMU GDB flag depending on version
QEMUGDB := $(shell if $(QEMU) -help | grep -q '^-gdb'; then echo "-gdb tcp::$(GDBPORT)"; else echo "-s -p $(GDBPORT)"; fi)

# Generate .gdbinit from template
.gdbinit: .gdbinit.tmpl
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

# Run QEMU paused and wait for GDB
qemu-gdb: $(ISO_DIR)/kernel.iso .gdbinit
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMU_FLAGS) $(ISO_DIR)/kernel.iso -S $(QEMUGDB)

# ==============================
# Cleanup
# ==============================
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ISO_DIR)/kernel.*
	rm -f $(ISO_DIR)/**/kernel.*
	rm -f serial.log
	rm -f $(VFS_CONFIG_GEN)

# ==============================
# Install dependencies (Linux/Debian)
# ==============================
install:
	sudo apt install -y grub-pc-bin grub-common xorriso mtools qemu-system-x86

