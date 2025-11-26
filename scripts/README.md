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

---

### `format_code.sh`

Formats ShipOS kernel code according to the project's coding style using `clang-format`.

**Usage:**
```bash
# Format all C/C++ files in kernel/
./scripts/format_code.sh

# Format specific files
./scripts/format_code.sh kernel/main.c kernel/sched/proc.c

# Check if files are formatted (don't modify)
./scripts/format_code.sh --check

# Check specific files
./scripts/format_code.sh --check kernel/main.c
```

**Options:**
- `-h, --help` - Show help message
- `-c, --check` - Check if files are formatted (don't modify)
- `-i, --in-place` - Format files in-place (default)
- `-a, --all` - Format all C/C++ files in kernel/ (default)

**Requirements:**
- `clang-format` (install with: `sudo apt-get install clang-format`)
- `.clang-format` configuration file in project root

**Configuration:**
The formatter uses a `.clang-format` configuration file in the project root that follows Allman style:
- 4-space indentation
- Opening braces on new line (Allman style)
- Right-aligned pointers (`type *name`)
- Space after keywords and around operators
- Preserves include order
- No line length limit

**Examples:**
```bash
# Format everything
./scripts/format_code.sh

# Check before committing
./scripts/format_code.sh --check

# Format only changed files
git diff --name-only | grep '\.c$\|\.h$' | xargs ./scripts/format_code.sh
```

