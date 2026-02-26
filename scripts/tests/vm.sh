#!/bin/bash
# Set of tests to check if VM functions are working fine

check() {
    if grep -q "$1 - Skipped" tests.log; then
        echo "$1 - Skipped"
    else 
        if grep -q "$1 - OK" tests.log; then
            echo "✅ $1 - OK"
        else 
            echo "❌ $1 - Failed"
            exit 1
        fi
    fi
}

check "VM: Page entry encode/decode"
check "VM: Page entry flags"
check "VM: kalloc returns aligned memory"
check "VM: kalloc/kfree consistency"
check "VM: Allocations are distinct"
check "VM: Memory write/read"
check "VM: memset fills correctly"
check "VM: Memory isolation"
check "VM: CR3 valid pagetable"
check "VM: Walk existing mapping"
check "VM: Walk allocates new entry"
check "VM: map_page works"
check "VM: va_to_pa translation"
check "VM: unmap_page works"
check "VM: map_pages range"
