#!/bin/bash
#
# Generate compile_commands.json for LSP support
# This script parses the Makefile and generates a compile_commands.json
# file that LSP servers can use to understand the build configuration.
#

set -e

# Get the project root directory (where this script is located)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Extract compiler and flags from Makefile
CC=$(grep '^CC :=' Makefile | sed 's/^CC := //' | tr -d '\r\n')
CFLAGS=$(grep '^CFLAGS :=' Makefile | sed 's/^CFLAGS := //' | sed 's/#.*$//' | tr -d '\r' | tr '\n' ' ' | sed 's/[[:space:]]*$//' | sed 's/  */ /g')

# Add include directory for kernel headers
# All includes are relative to kernel/, so we add kernel as include path
INCLUDE_FLAGS="-Ikernel"

# Temporary file for building JSON
TMP_JSON=$(mktemp)
trap "rm -f $TMP_JSON" EXIT

# Start building the JSON array
echo "[" > "$TMP_JSON"

# Find all C source files in kernel/
FIRST=true
while IFS= read -r -d '' file; do
    # Get relative path from project root
    rel_file="${file#$PROJECT_ROOT/}"

    # Build the output object file path (matching Makefile pattern)
    obj_file="build/kernel/${rel_file#kernel/}"
    obj_file="${obj_file%.c}.o"

    # Build the compile command (ensure single line)
    # CFLAGS already includes -c, so we don't need to add it again
    command="$CC $CFLAGS $INCLUDE_FLAGS -o $obj_file $rel_file"

    # Add comma separator if not first entry
    if [ "$FIRST" = true ]; then
        FIRST=false
    else
        echo "," >> "$TMP_JSON"
    fi

    # Write JSON entry using jq if available, otherwise manual JSON
    if command -v jq >/dev/null 2>&1; then
        jq -n \
            --arg dir "$PROJECT_ROOT" \
            --arg cmd "$command" \
            --arg file "$rel_file" \
            '{directory: $dir, command: $cmd, file: $file}' >> "$TMP_JSON"
    else
        # Manual JSON construction (escape quotes and backslashes)
        dir_escaped=$(echo "$PROJECT_ROOT" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g')
        cmd_escaped=$(echo "$command" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g')
        file_escaped=$(echo "$rel_file" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g')

        cat >> "$TMP_JSON" << EOF
  {
    "directory": "$dir_escaped",
    "command": "$cmd_escaped",
    "file": "$file_escaped"
  }
EOF
    fi

done < <(find kernel -name "*.c" -type f -print0 | sort -z)

# Close the JSON array
echo "" >> "$TMP_JSON"
echo "]" >> "$TMP_JSON"

# Validate JSON before writing
if command -v python3 >/dev/null 2>&1; then
    if python3 -m json.tool "$TMP_JSON" > /dev/null 2>&1; then
        mv "$TMP_JSON" compile_commands.json
        echo "Generated compile_commands.json"
        echo "Location: $PROJECT_ROOT/compile_commands.json"
        echo "Found $(grep -c '"file"' compile_commands.json) C source files"
    else
        echo "Error: Generated invalid JSON"
        python3 -m json.tool "$TMP_JSON" 2>&1 | head -5
        exit 1
    fi
else
    # If python3 not available, just copy (user can validate manually)
    mv "$TMP_JSON" compile_commands.json
    echo "Generated compile_commands.json (not validated - python3 not found)"
    echo "Location: $PROJECT_ROOT/compile_commands.json"
    echo "Found $(grep -c '"file"' compile_commands.json) C source files"
fi
