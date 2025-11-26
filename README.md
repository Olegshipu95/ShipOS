# ShipOS
Import substitution of foreign products. ShipOS power

## Contributing

We welcome contributions to ShipOS! However, **all contributors must sign our [Contributor License Agreement (CLA)](CLA.md)** before their contributions can be accepted.

**Important:** By contributing to ShipOS, you agree that all your contributions (past, present, and future) become the exclusive property of ShipOS. This means ShipOS retains full rights to use, modify, distribute, and commercialize your contributions.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines.

## How to use

```bash
 make install
 make qemu
```

Implemented parts of the operating system
1) Bootloader, long jump to x64
2) Minimal keyboard handling through static interrupts table
3) Virtual terminals and text output
4) Memory management: Initial memory map
5) Memory management: Paging
6) Memory management: malloc-level memory manager
7) Interruptions: basic hard, soft handling
8) Interruptions: err, trap handling and context info collection
9) Interruptions: basic keyboard & virtual terminal
10) Threading: thread context save/restore procuderes
11) Threading: thread context initialization and autoswitching

Roadmap
1) Userspace: Process(mem + threads)
2) Userspace: IOPL switching
3) Userspace: syscall mechanics
