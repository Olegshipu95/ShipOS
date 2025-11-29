# ShipOS

**ShipOS** is a modern, educational operating system kernel written from scratch for x86-64 architecture. The project aims to provide a complete, production-quality operating system as an alternative to foreign products, demonstrating import substitution capabilities while serving as a comprehensive learning resource for low-level systems programming.

## About the Project

ShipOS is a fully functional operating system kernel that implements core OS concepts including memory management, process scheduling, interrupt handling, and device drivers. Built with educational and practical goals in mind, ShipOS demonstrates how modern operating systems work at the lowest level.

### Why ShipOS?

1. **Import Substitution**: ShipOS provides a domestic alternative to foreign operating systems, reducing dependency on external software products.

2. **Educational Value**: The codebase serves as a comprehensive learning resource for understanding operating system internals, from bootloader to kernel subsystems.

3. **Full Control**: By building from scratch, ShipOS maintains complete control over its architecture, security model, and feature set without external dependencies.

4. **Research Platform**: The project provides a clean foundation for experimenting with new OS concepts, scheduling algorithms, and system designs.

5. **Production Potential**: With proper development, ShipOS can evolve into a production-ready operating system suitable for specialized use cases.

## Current Status

### ✅ Implemented Features

ShipOS currently implements the following core subsystems:

#### Boot and Initialization
- **Bootloader**: Custom bootloader with long jump to x86-64 mode
- **Kernel Entry**: Proper kernel initialization and setup
- **Serial Port**: COM1 serial port driver for logging and debugging

#### Memory Management
- **Physical Memory Allocator**: Initial memory map setup and management
- **Virtual Memory**: Complete paging implementation with page table management
- **Kernel Memory**: Malloc-level memory manager for kernel allocations

#### Interrupt Handling
- **IDT Setup**: Interrupt Descriptor Table initialization
- **Hardware Interrupts**: Basic hardware interrupt handling
- **Software Interrupts**: Software interrupt support
- **Exception Handling**: Error and trap handling with context information collection
- **PIC/PIT**: Programmable Interrupt Controller and Timer initialization

#### Process and Thread Management
- **Thread Context**: Thread context save/restore procedures
- **Thread Initialization**: Thread context initialization
- **Thread Switching**: Automatic thread switching and scheduling
- **Process Structure**: Process management framework

#### I/O and Terminal
- **VGA Driver**: VGA text mode driver for display output
- **TTY Subsystem**: Virtual terminal support with multiple terminals
- **Keyboard Handling**: Basic keyboard input through interrupt-driven interface

#### Synchronization
- **Spinlocks**: Low-level synchronization primitives
- **Mutexes**: Mutual exclusion locks for thread synchronization

### 🚧 Planned Features (Roadmap)

- **Userspace**: Process isolation with separate memory spaces
- **IOPL Switching**: I/O privilege level switching for user processes
- **System Calls**: Complete syscall mechanism for userspace interaction
- **File System**: Persistent storage and file management
- **Network Stack**: Network protocol implementation
- **Device Drivers**: Additional hardware driver support

## Quick Start

### Prerequisites

- Linux-based development environment
- `gcc` (GNU Compiler Collection)
- `nasm` (Netwide Assembler)
- `grub-pc-bin` and `xorriso` (for ISO creation)
- `qemu-system-x86_64` (for emulation)

### Building and Running

```bash
# Install dependencies (one-time setup)
make install

# Build the kernel and create bootable ISO
make build_iso

# Run in QEMU with GUI
make qemu

# Run in headless mode (logs to serial.log)
make qemu-headless
```

For detailed development instructions, debugging, and advanced usage, see [DEVELOPMENT.md](DEVELOPMENT.md).

## Project Structure

```
ShipOS/
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

## Documentation

- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Development guide, build system, debugging, and tools
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Contribution guidelines and development workflow
- **[CONTRIBUTOR_ASSIGNMENT.md](CONTRIBUTOR_ASSIGNMENT.md)** - Contributor Assignment Agreement (required for all contributors)

## Contributing

We welcome contributions to ShipOS! However, **all contributors must sign our [Contributor Assignment Agreement](CONTRIBUTOR_ASSIGNMENT.md)** before their contributions can be accepted.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines, code style requirements, and development workflow.

## Legal Information

### License and Ownership

ShipOS is a proprietary project. All code, documentation, and contributions are the exclusive property of the ShipOS project owner.

**Important Legal Notice:**

- **Contributor Assignment Agreement**: All contributors must sign the [Contributor Assignment Agreement](CONTRIBUTOR_ASSIGNMENT.md) before contributing.

- **Exclusive Rights**: ShipOS retains full rights to:
  - Close or restrict access to the repository
  - Prohibit distribution of the code
  - Sell, license, or commercialize the project
  - Modify, remove, or reject any contribution
  - Change license terms at any time

For complete legal terms, please read the [Contributor Assignment Agreement](CONTRIBUTOR_ASSIGNMENT.md).

### Why we use a Contributor Assignment Agreement (CAA)

We use a Contributor Assignment Agreement (CAA) to ensure the project’s stability and future development. It helps avoid legal issues and allows all contributions to be managed under a single license, so the project can grow consistently and securely over time.

## Copyright

Copyright (c) 2023 SHIPOS. All rights reserved.

## Support

- **Issues**: Report bugs or request features via [GitHub Issues](https://github.com/YOUR_USERNAME/ShipOS/issues)
- **Development Questions**: See [DEVELOPMENT.md](DEVELOPMENT.md)
- **Contribution Questions**: See [CONTRIBUTING.md](CONTRIBUTING.md)

---

**ShipOS** - We stealed your threads
