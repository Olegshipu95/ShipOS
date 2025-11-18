NASM=nasm
NASM_FLAGS=-f elf64

LD=ld
LD_FLAGS=--nmagic --script=$(LINKER)

CC=gcc 
CFLAGS=-Wall -c -ggdb -ffreestanding -mgeneral-regs-only

GRUB=grub-mkrescue
GRUB_FLAGS=-o

LINKER=x86_64/boot/linker.ld

ISO_DIR=isofiles
ISO_BOOT_DIR := $(ISO_DIR)/boot

BUILD_DIR=build

x86_64_asm_source_files := $(shell find x86_64 -name *.asm)
x86_64_asm_object_files := $(patsubst x86_64/%.asm, build/x86_64/%.o, $(x86_64_asm_source_files))

kernel_source_files := $(shell find kernel -name *.c)
kernel_object_files := $(patsubst kernel/%.c, build/kernel/%.o, $(kernel_source_files))

kernel_asm_source_files := $(shell find kernel -name *.asm)
kernel_asm_object_files := $(patsubst kernel/%.asm, build/kernel/%.o, $(kernel_asm_source_files))

object_files := $(x86_64_asm_object_files) $(kernel_object_files) $(kernel_asm_object_files)

build/%.o: %.asm
	mkdir -p $(dir $@) && \
	$(NASM) $(NASM_FLAGS) $< -o $@

build/%.o: %.c
	mkdir -p $(dir $@) && \
	$(CC) $(CFLAGS) $< -o $@

$(ISO_BOOT_DIR)/kernel.bin: $(object_files)
	$(LD) $(LD_FLAGS) --output=$@ $^

$(ISO_DIR)/kernel.iso: $(ISO_BOOT_DIR)/kernel.bin
	$(GRUB) $(GRUB_FLAGS) $@ $(ISO_DIR)

build_kernel: $(ISO_BOOT_DIR)/kernel.bin
build_iso: $(ISO_DIR)/kernel.iso

QEMU=qemu-system-x86_64
QEMU_FLAGS=-m 128M -cdrom

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)

# -s -S -kernel
# tap adapter 

qemu: $(ISO_DIR)/kernel.iso
	$(QEMU)  \
	$(QEMU_FLAGS) \
	$(ISO_DIR)/kernel.iso

.gdbinit: .gdbinit.tmpl
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $(ISO_DIR)/kernel.iso .gdbinit
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMU_FLAGS) $(ISO_DIR)/kernel.iso -S $(QEMUGDB)

clean: 
	rm -rf $(BUILD_DIR)
	rm -f $(ISO_DIR)/kernel.*
	rm -f $(ISO_DIR)/**/kernel.*

install:
	apt install grub-pc-bin
	apt install grub-common
	apt install xorriso
	apt install mtools
	apt install qemu-system-x86 
