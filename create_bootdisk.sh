#!/bin/bash

# Create bootable disk image for MyKernel
echo "Creating bootable disk image for MyKernel..."

# Check if kernel exists
if [ ! -f "mykernel.bin" ]; then
    echo "Error: mykernel.bin not found. Run 'make' first."
    exit 1
fi

# Assemble MBR
echo "Assembling MBR bootloader..."
i686-elf-as boot/mbr.s -o boot/mbr.o
i686-elf-ld -Ttext 0x7C00 --oformat binary -o boot/mbr.bin boot/mbr.o

# Create disk image (10MB)
echo "Creating disk image..."
dd if=/dev/zero of=mykernel_boot.img bs=1M count=10 2>/dev/null

# Write MBR to first sector
echo "Writing MBR..."
dd if=boot/mbr.bin of=mykernel_boot.img bs=512 count=1 conv=notrunc 2>/dev/null

# Write kernel starting from sector 2
echo "Writing kernel..."
dd if=mykernel.bin of=mykernel_boot.img bs=512 seek=1 conv=notrunc 2>/dev/null

echo "Bootable disk created: mykernel_boot.img"
echo ""
echo "To test with QEMU:"
echo "  qemu-system-i386 -drive format=raw,file=mykernel_boot.img"
echo ""
echo "To write to real hardware (USB/disk):"
echo "  sudo dd if=mykernel_boot.img of=/dev/sdX bs=1M"
echo "  (Replace /dev/sdX with your target device - BE VERY CAREFUL!)"
echo ""
echo "WARNING: Writing to wrong device will destroy data!"

# Test with QEMU if available
if command -v qemu-system-i386 &> /dev/null; then
    echo ""
    read -p "Test with QEMU now? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Starting QEMU..."
        qemu-system-i386 -drive format=raw,file=mykernel_boot.img -netdev user,id=net0 -device rtl8139,netdev=net0
    fi
fi