#!/bin/bash

set -e

DISK_IMG="tmp/disk.img"
EFI_FILE="EFI/BOOTX64.EFI"
KERNEL_FILE="build/kernel_uefi.bin"
DISK_SIZE_MB=64
STARTUP="startup.nsh"

echo "fs0:\\EFI\\BOOT\\BOOTX64.EFI" > $STARTUP

mkdir tmp

dd if=/dev/zero of="$DISK_IMG" bs=1M count=$DISK_SIZE_MB status=progress
mkfs.fat -F 32 "$DISK_IMG"

mmd -i "$DISK_IMG" ::/EFI
mmd -i "$DISK_IMG" ::/EFI/BOOT

mcopy -i "$DISK_IMG" "$EFI_FILE" ::/EFI/BOOT/BOOTX64.EFI
mcopy -i "$DISK_IMG" "$KERNEL_FILE" ::/kernel.bin
mcopy -i "$DISK_IMG" "$STARTUP" ::/

mdir -i "$DISK_IMG" ::/

rm -f $STARTUP
