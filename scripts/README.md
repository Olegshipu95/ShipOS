# Scripts Directory

This directory contains utility scripts for the ShipOS project.

## Scripts

### `gen_compile_commands.sh`

Generates `compile_commands.json` for LSP (Language Server Protocol) support.

**Usage:**
```bash
./scripts/gen_compile_commands.sh
```

This script:
- Parses the Makefile to extract compiler and flags
- Finds all C source files in the `kernel/` directory
- Generates a `compile_commands.json` file in the project root
- Validates the generated JSON

**Requirements:**
- `bash`
- `python3` (optional, for JSON validation)
- `jq` (optional, for better JSON generation)

**Output:**
- `compile_commands.json` in the project root directory

This file enables LSP servers (like clangd, ccls, etc.) to understand your build configuration and provide accurate code completion, error checking, and navigation.

