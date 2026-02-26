#!/bin/sh

echo "🔍 Checking for expected boot messages..."

check() {
    if grep -q "$2" report.log; then
        echo "✅ $1"
    else
        echo "❌ $1 missing!"
        exit 1
    fi
}

# Core initialization
check "Serial port initialized" "\[SERIAL\] Serial ports initialized successfully"
check "TTY subsystem initialized" "\[BOOT\] TTY subsystem initialized"

# Memory subsystem
check "kinit complete" "\[MEMORY\] kinit complete"
check "kvminit complete" "\[MEMORY\] kvminit complete"
check "Physical memory initialized" "\[MEMORY\] Physical memory initialized"

# ACPI/Hardware detection
check "RSDP found" "\[RSDP\] Found"
check "RSDT initialized" "\[RSDT\] Initialized"
check "CPU detection" "\[CPU\] Detected"
check "Local APIC mapped" "\[MEMORY\] Mapping Local APIC"
check "I/O APIC mapped" "\[MEMORY\] Mapping I/O APIC"

# Per-CPU initialization
check "Per-CPU structures initialized" "\[PERCPU\] Per-CPU data structures initialized"

# Scheduler initialization
check "SMP scheduler initialized" "\[SCHED\] SMP scheduler initialized"
check "CPU 0 scheduler initialized" "\[SCHED\] CPU 0 scheduler initialized"

# Interrupt system
check "LAPIC initialized" "\[LAPIC\] Initialized"
check "IOAPIC initialized" "\[IOAPIC\] Initialized"
check "Keyboard IRQ enabled" "\[IDT\] Enabling keyboard IRQ1"

# Boot completion
check "Kernel boot completed" "\[KERNEL\] Boot sequence completed successfully"

# SMP startup
check "Application Processors started" "\[KERNEL\] Started .* Application Processors"

echo "🎉 All boot checks passed!"