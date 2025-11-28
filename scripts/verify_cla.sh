#!/bin/bash
#
# Helper script for maintainers to verify CLA signatures
# This is a simple tool - for full automation, use CLA Assistant App
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Default signature storage location (can be overridden)
SIGNATURES_DIR="${CLA_SIGNATURES_DIR:-.github/cla-signatures}"

usage() {
    cat << EOF
Usage: $0 [OPTIONS] [USERNAME]

Verify if a contributor has signed the CLA.

Options:
    -h, --help          Show this help message
    -l, --list          List all signed contributors
    -c, --check USER    Check if specific user has signed
    -a, --add USER      Add a signature manually (for maintainers)
    -d, --dir DIR       Use custom signatures directory (default: $SIGNATURES_DIR)

Examples:
    $0 --check username        # Check if user has signed
    $0 --list                  # List all signed contributors
    $0 --add username          # Manually add signature (use with caution)

EOF
}

check_signature() {
    local username="$1"
    
    if [ -z "$username" ]; then
        echo "Error: Username required"
        usage
        exit 1
    fi
    
    # Check in signatures directory
    if [ -d "$SIGNATURES_DIR" ]; then
        # Look for signature file
        local sig_file=$(find "$SIGNATURES_DIR" -name "${username}.json" -o -name "${username}.txt" 2>/dev/null | head -1)
        
        if [ -n "$sig_file" ]; then
            echo "✅ $username has signed the CLA"
            echo "   Signature file: $sig_file"
            if [ -f "$sig_file" ]; then
                echo "   Details:"
                cat "$sig_file" | head -10
            fi
            return 0
        fi
    fi
    
    # Check git log for CLA-related commits/comments
    if git log --all --grep="CLA" --author="$username" --oneline | head -1 > /dev/null 2>&1; then
        echo "⚠️  Found CLA-related activity for $username, but no signature file"
        echo "   Please verify manually"
        return 1
    fi
    
    echo "❌ No CLA signature found for $username"
    return 1
}

list_signatures() {
    if [ ! -d "$SIGNATURES_DIR" ]; then
        echo "Signatures directory not found: $SIGNATURES_DIR"
        echo "Set CLA_SIGNATURES_DIR environment variable to specify location"
        return 1
    fi
    
    echo "Signed Contributors:"
    echo "===================="
    
    local count=0
    while IFS= read -r -d '' file; do
        local username=$(basename "$file" .json)
        username=$(basename "$username" .txt)
        echo "  ✓ $username"
        count=$((count + 1))
    done < <(find "$SIGNATURES_DIR" -type f \( -name "*.json" -o -name "*.txt" \) -print0 2>/dev/null)
    
    if [ $count -eq 0 ]; then
        echo "  (no signatures found)"
    else
        echo ""
        echo "Total: $count signed contributor(s)"
    fi
}

add_signature() {
    local username="$1"
    
    if [ -z "$username" ]; then
        echo "Error: Username required"
        usage
        exit 1
    fi
    
    echo "⚠️  WARNING: Manual signature addition"
    echo "This should only be used for verified signatures."
    echo ""
    read -p "Continue? (yes/no): " confirm
    
    if [ "$confirm" != "yes" ]; then
        echo "Cancelled"
        exit 0
    fi
    
    mkdir -p "$SIGNATURES_DIR"
    
    local sig_file="$SIGNATURES_DIR/${username}.json"
    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    
    cat > "$sig_file" << EOF
{
  "username": "$username",
  "signedAt": "$timestamp",
  "claVersion": "1.0",
  "addedBy": "manual",
  "verified": true,
  "note": "Manually added by maintainer"
}
EOF
    
    echo "✅ Added signature for $username"
    echo "   File: $sig_file"
}

# Parse arguments
case "${1:-}" in
    -h|--help)
        usage
        exit 0
        ;;
    -l|--list)
        list_signatures
        ;;
    -c|--check)
        if [ -z "$2" ]; then
            echo "Error: Username required for --check"
            usage
            exit 1
        fi
        check_signature "$2"
        ;;
    -a|--add)
        if [ -z "$2" ]; then
            echo "Error: Username required for --add"
            usage
            exit 1
        fi
        add_signature "$2"
        ;;
    -d|--dir)
        if [ -z "$2" ]; then
            echo "Error: Directory required for --dir"
            usage
            exit 1
        fi
        SIGNATURES_DIR="$2"
        shift
        if [ -n "$2" ]; then
            case "$2" in
                -c|--check)
                    check_signature "$3"
                    ;;
                -l|--list)
                    list_signatures
                    ;;
                *)
                    usage
                    exit 1
                    ;;
            esac
        fi
        ;;
    "")
        usage
        exit 0
        ;;
    *)
        # Assume it's a username
        check_signature "$1"
        ;;
esac

