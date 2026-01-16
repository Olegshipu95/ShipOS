# UEFI 

to load **ShipOS** using uefi in headless mode:

```bash
# Install dependencies (one-time setup)
make install

# Clean build files
make clean

# Run in headless mode (logs to stdio)
make qemu-uefi
```

