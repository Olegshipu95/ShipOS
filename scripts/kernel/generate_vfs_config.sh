#!/bin/bash
#
# Script to generate vfs_config.c from configs/fs.conf
# Format: <mount_point>:<fs_type>[:<device>]
#
# Usage: generate_vfs_config.sh <config_file> <output_file>

set -e

CONFIG_FILE="$1"
OUTPUT_FILE="$2"

if [ -z "$CONFIG_FILE" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "Usage: $0 <config_file> <output_file>" >&2
    exit 1
fi

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Config file '$CONFIG_FILE' not found" >&2
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$(dirname "$OUTPUT_FILE")"

# Generate C file
cat > "$OUTPUT_FILE" << 'EOF'
//
// Auto-generated file. Do not edit manually.
// Generated from configs/fs.conf by scripts/generate_vfs_config.sh
//

#include "kernel/vfs/configs/vfs_config.h"

struct vfs_mount_config vfs_mount_configs[] = {
EOF

# Parse configuration file
while IFS= read -r line || [ -n "$line" ]; do
    # Skip comments and empty lines
    line_trimmed=$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    [[ "$line_trimmed" =~ ^#.*$ ]] && continue
    [[ -z "$line_trimmed" ]] && continue
    
    # Parse format: mount_point:fs_type[:device]
    IFS=':' read -r mount_point fs_type device <<< "$line_trimmed"
    
    # Trim whitespace
    mount_point=$(echo "$mount_point" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    fs_type=$(echo "$fs_type" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    device=$(echo "$device" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    
    # Validate required fields
    if [ -z "$mount_point" ] || [ -z "$fs_type" ]; then
        echo "Warning: Skipping invalid line: $line" >&2
        continue
    fi
    
    # Generate C struct entry
    if [ -z "$device" ]; then
        echo "    { \"$mount_point\", \"$fs_type\", NULL }," >> "$OUTPUT_FILE"
    else
        echo "    { \"$mount_point\", \"$fs_type\", \"$device\" }," >> "$OUTPUT_FILE"
    fi
done < "$CONFIG_FILE"

cat >> "$OUTPUT_FILE" << 'EOF'
    { NULL, NULL, NULL }  // Terminator
};

size_t vfs_mount_configs_count = sizeof(vfs_mount_configs) / sizeof(vfs_mount_configs[0]) - 1;
EOF

echo "Generated $OUTPUT_FILE from $CONFIG_FILE"

