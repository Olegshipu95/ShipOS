#!/bin/bash
#
# Format untitled-os kernel code according to Allman style
# Uses clang-format with .clang-format configuration
# Allman style: Opening braces on new line
#

set -e

# Get the project root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Check if clang-format is available
if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format is not installed"
    echo "Install it with: sudo apt-get install clang-format"
    exit 1
fi

# Check if .clang-format exists
if [ ! -f ".clang-format" ]; then
    echo "Error: .clang-format configuration file not found"
    exit 1
fi

# Default: format all C and header files
FORMAT_ALL=true
FILES=()

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "Usage: $0 [OPTIONS] [FILES...]"
            echo ""
            echo "Format untitled-os kernel code according to Allman style (opening braces on new line)."
            echo ""
            echo "Options:"
            echo "  -h, --help          Show this help message"
            echo "  -c, --check         Check if files are formatted (don't modify)"
            echo "  -i, --in-place     Format files in-place (default)"
            echo "  -a, --all          Format all C/C++ files in kernel/ (default)"
            echo ""
            echo "Examples:"
            echo "  $0                          # Format all files"
            echo "  $0 kernel/main.c            # Format specific file"
            echo "  $0 -c                       # Check all files"
            echo "  $0 -c kernel/main.c         # Check specific file"
            exit 0
            ;;
        -c|--check)
            CHECK_MODE=true
            shift
            ;;
        -i|--in-place)
            CHECK_MODE=false
            shift
            ;;
        -a|--all)
            FORMAT_ALL=true
            shift
            ;;
        *)
            FILES+=("$1")
            FORMAT_ALL=false
            shift
            ;;
    esac
done

# Set default mode
CHECK_MODE=${CHECK_MODE:-false}

# Find files to format
if [ "$FORMAT_ALL" = true ]; then
    # Find all C and header files
    FILES=($(find kernel -type f \( -name "*.c" -o -name "*.h" \) | sort))
fi

if [ ${#FILES[@]} -eq 0 ]; then
    echo "No files to format"
    exit 0
fi

# Format files
FORMATTED_COUNT=0
NEEDS_FORMAT_COUNT=0

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "Warning: File not found: $file"
        continue
    fi

    if [ "$CHECK_MODE" = true ]; then
        # Check if file needs formatting
        if ! clang-format "$file" | diff -q "$file" - >/dev/null 2>&1; then
            echo "❌ $file needs formatting"
            NEEDS_FORMAT_COUNT=$((NEEDS_FORMAT_COUNT + 1))
        else
            echo "✅ $file is properly formatted"
        fi
    else
        # Format in-place
        clang-format -i "$file"
        echo "Formatted: $file"
        FORMATTED_COUNT=$((FORMATTED_COUNT + 1))
    fi
done

# Print summary
if [ "$CHECK_MODE" = true ]; then
    echo ""
    if [ $NEEDS_FORMAT_COUNT -eq 0 ]; then
        echo "✅ All files are properly formatted!"
        exit 0
    else
        echo "❌ $NEEDS_FORMAT_COUNT file(s) need formatting"
        echo "Run '$0' to format them"
        exit 1
    fi
else
    echo ""
    echo "✅ Formatted $FORMATTED_COUNT file(s)"
fi