# untitled-os Development Guide

This guide explains the development workflow, build system, and debugging tools for untitled-os.

## Build System

### Make Targets

```bash
# Install dependencies (one-time setup)
make install

# Build kernel binary only
make build_kernel

# Build bootable ISO
make build_iso

# Run QEMU with GUI
make qemu

# Run QEMU in headless mode (logs to serial.log)
make qemu-headless

# Run QEMU paused, waiting for GDB connection
make qemu-gdb

# Clean build artifacts
make clean
```

### Build Process

1. **Compilation**: C files → object files (`.o`)
2. **Assembly**: ASM files → object files (`.o`)
3. **Linking**: All objects → `kernel.bin`
4. **ISO Creation**: `kernel.bin` + GRUB → `kernel.iso`

Build artifacts are in `build/` directory. The final ISO is `isofiles/kernel.iso`.

## Debugging

### GDB Debugging

untitled-os uses a unique GDB port per user (based on user ID) to avoid conflicts.

**Setup:**
```bash
# Terminal 1: Start QEMU paused, waiting for GDB
make qemu-gdb

# Terminal 2: Connect GDB
gdb
(gdb) target remote localhost:$(expr `id -u` % 5000 + 25000)
(gdb) file isofiles/boot/kernel.bin
(gdb) symbol-file isofiles/boot/kernel.bin
(gdb) break kernel_main
(gdb) continue
```

**Common GDB Commands:**
```gdb
# Set breakpoints
break kernel_main
break panic

# Step through code
step
next
continue

# Inspect memory
x/10x $rsp
print/x $rax

# View registers
info registers

# Backtrace
bt
```

The GDB port is automatically calculated: `25000 + (user_id % 5000)`.

### Serial Port Logging

For headless debugging and CI, use serial port logging:

```bash
# Run in headless mode
make qemu-headless

# View logs in real-time
tail -f serial.log

# Or check logs after QEMU stops
cat serial.log
```

All kernel output (including panics) is logged to `serial.log` when using headless mode.

### Bochs Emulator

Bochs provides cycle-accurate x86 emulation, useful for low-level debugging.

**Configuration:**
- Config file: `bochsrc.bxrc`
- Boots from: `isofiles/kernel.iso`
- GUI debugger enabled

**Usage:**
```bash
# Install Bochs (if not installed)
sudo apt install bochs bochs-x

# Run Bochs
bochs -f bochsrc.bxrc
```

**Bochs Debugger:**
- Press `Ctrl+C` in Bochs window to enter debugger
- Use `help` for available commands
- `info registers` - view CPU state
- `x /10x 0x100000` - examine memory
- `c` - continue execution

## Development Tools

### Code Formatting

Format code according to Allman style:

```bash
# Format all files
./scripts/format_code.sh

# Format specific files
./scripts/format_code.sh kernel/main.c

# Check formatting without modifying
./scripts/format_code.sh --check
```

### LSP Support

Generate `compile_commands.json` for IDE/LSP support:

```bash
./scripts/gen_compile_commands.sh
```

This enables code completion, error checking, and navigation in editors like VSCode, Vim, etc.

## Project Structure

```
untitled-os/
├── kernel/          # Kernel source code
│   ├── idt/         # Interrupt Descriptor Table
│   ├── kalloc/      # Physical memory allocator
│   ├── paging/      # Virtual memory management
│   ├── sched/       # Process/thread scheduling
│   ├── serial/      # Serial port driver
│   ├── sync/        # Synchronization primitives
│   ├── tty/         # Terminal subsystem
│   └── vga/         # VGA text mode driver
├── x86_64/          # Bootloader and low-level code
├── scripts/         # Development utilities
└── isofiles/        # Build output (ISO, kernel.bin)
```

## Quick Start

```bash
# 1. Install dependencies
make install

# 2. Build and run
make build_iso
make qemu

# 3. For debugging
make qemu-gdb    # In one terminal
gdb              # In another terminal
```

## Tips

- Use `make qemu-headless` for CI/testing (captures all output)
- Serial logs include panic messages even after crashes
- GDB port is user-specific to avoid conflicts in multi-user systems
- Bochs is slower but more accurate than QEMU for low-level debugging
- Format code before committing: `./scripts/format_code.sh`

