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

OBJCOPY := objcopy
OBJCOPY_FLAGS := -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym \
				 -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
				 --target=efi-app-x86_64

# ==============================
# Directories
# ==============================
ISO_DIR := isofiles
ISO_BOOT_DIR := $(ISO_DIR)/boot
BUILD_DIR := build
EFI_DIR := EFI
TMP_DIR := tmp

# UEFI-specific linker
LINKER_UEFI := uefi/linker_uefi.ld
LD_FLAGS_UEFI := --nmagic --script=$(LINKER_UEFI)

# ==============================
# UEFI Toolchain
# ==============================
EFI_CC := gcc
EFI_CFLAGS := -I/usr/include/efi \
			  -I/usr/include/efi/x86_64 \
			  -I/usr/include/efi/protocol \
			  -fpic \
			  -ffreestanding \
			  -fno-stack-protector \
			  -fno-stack-check \
			  -fshort-wchar \
			  -mno-red-zone \
			  -maccumulate-outgoing-args \
			  -DEFI_FUNCTION_WRAPPER \
			  -Wall
EFI_LD := ld
EFI_LDFLAGS := -nostdlib \
			   -znocombreloc \
			   -shared \
			   -Bsymbolic \
			   -L /usr/lib/ \
			   -T /usr/lib/elf_x86_64_efi.lds /usr/lib//crt0-efi-x86_64.o

EFI_LIBS := -lefi -lgnuefi

OVMF := ovmf/OVMF_CODE.fd
OVMF_VARS := ovmf/OVMF_VARS-1024x768.fd

QEMU_UEFI_FLAGS := -m 256M \
	-drive if=pflash,format=raw,unit=0,readonly=on,file=$(OVMF) \
	-nographic

# ==============================
# Source files
# ==============================
# BIOS boot assembly (32-bit entry point)
bios_boot_sources := x86_64/boot/boot.asm x86_64/boot/header.asm x86_64/boot/check_functions.asm
bios_boot_objects := $(patsubst x86_64/boot/%.asm,$(BUILD_DIR)/x86_64/boot/%.o,$(bios_boot_sources))

# All C files in kernel/
kernel_c_sources := $(shell find kernel -name '*.c')
kernel_c_objects := $(patsubst kernel/%.c,$(BUILD_DIR)/kernel/%.o,$(kernel_c_sources))

# All assembly files in kernel/
kernel_asm_sources := $(shell find kernel -name '*.asm')
kernel_asm_objects := $(patsubst kernel/%.asm,$(BUILD_DIR)/kernel/%.o,$(kernel_asm_sources))

# Objects for BIOS boot (includes 32-bit bootstrap)
bios_objects := $(bios_boot_objects) $(kernel_c_objects) $(kernel_asm_objects)

# Objects for UEFI boot - now only kernel code, bootloader handles entry
uefi_objects := $(kernel_c_objects) $(kernel_asm_objects)

# ==============================
# Build rules
# ==============================

# Compile assembly files
$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_FLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# Link all object files into kernel binary (BIOS boot)
$(ISO_BOOT_DIR)/kernel.bin: $(bios_objects)
	@mkdir -p $(ISO_BOOT_DIR)
	$(LD) $(LD_FLAGS) --output=$@ $^

# Link kernel for UEFI boot
$(BUILD_DIR)/kernel_uefi.elf: $(uefi_objects)
	@mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS_UEFI) --output=$@ $^

# Create bootable ISO using GRUB
$(ISO_DIR)/kernel.iso: $(ISO_BOOT_DIR)/kernel.bin
	$(GRUB) $(GRUB_FLAGS) $@ $(ISO_DIR)

# Create EFI app
$(EFI_DIR)/BOOTX64.EFI: $(BUILD_DIR)/uefi/bootloader.so
	@mkdir -p $(dir $@)
	$(OBJCOPY) $(OBJCOPY_FLAGS) $< $@

# Build UEFI bootloader (C-based, no asm entry point needed)
$(BUILD_DIR)/uefi/bootloader.so: uefi/bootloader.c
	@mkdir -p $(dir $@)
	$(EFI_CC) $(EFI_CFLAGS) $< -c -o $(BUILD_DIR)/uefi/bootloader.o
	$(EFI_LD) $(EFI_LDFLAGS) $(BUILD_DIR)/uefi/bootloader.o -o $@ $(EFI_LIBS)

# Create IMG disk file for UEFI start (use ELF, not flat binary)
$(TMP_DIR)/disk.img: $(BUILD_DIR)/kernel_uefi.elf $(EFI_DIR)/BOOTX64.EFI
	uefi/create_disk_image.sh

# ==============================
# Phony targets
# ==============================
.PHONY: build_kernel build_iso build_uefi qemu qemu-uefi qemu-headless clean install

# Build kernel binary only
build_kernel: $(ISO_BOOT_DIR)/kernel.bin

# Build ISO
build_iso: $(ISO_DIR)/kernel.iso

# Build UEFI disk image
build_uefi: $(TMP_DIR)/disk.img
	cp $(OVMF_VARS) $(TMP_DIR)/OVMF_VARS.tmp

# Run QEMU in BIOS mode
qemu: $(ISO_DIR)/kernel.iso
	$(QEMU) $(QEMU_FLAGS) $(ISO_DIR)/kernel.iso

# Run QEMU in UEFI mode
qemu-uefi: build_uefi
	$(QEMU) $(QEMU_UEFI_FLAGS) -drive format=raw,file=$(TMP_DIR)/disk.img

# Run QEMU in Headless mode (no GUI, logs to serial.log, BIOS)
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
	rm -rf $(EFI_DIR)
	rm -rf $(TMP_DIR)
	rm -f serial.log

# ==============================
# Install dependencies (Linux/Debian)
# ==============================
install:
	sudo apt install -y grub-pc-bin grub-common xorriso mtools qemu-system-x86 ovmf gnu-efi dosfstools
