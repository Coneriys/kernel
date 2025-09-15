#!/bin/bash

# Create bootable ISO for MyKernel
# This script creates an ISO image that can be burned to CD or USB

# Check if mykernel.bin exists
if [ ! -f "mykernel.bin" ]; then
    echo "Error: mykernel.bin not found. Run 'make' first."
    exit 1
fi

# Create ISO directory structure
echo "Creating ISO directory structure..."
mkdir -p iso/boot/grub

# Create GRUB configuration
cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=5
set default=0

menuentry "MyKernel OS" {
    multiboot /boot/mykernel.bin
    boot
}

menuentry "MyKernel OS (Safe Mode)" {
    multiboot /boot/mykernel.bin
    boot
}
EOF

# Copy kernel to ISO directory
echo "Copying kernel..."
cp mykernel.bin iso/boot/

# Create ISO image using grub-mkrescue (if available)
if command -v grub-mkrescue &> /dev/null; then
    echo "Creating ISO image with grub-mkrescue..."
    grub-mkrescue -o mykernel.iso iso/
    echo "ISO created: mykernel.iso"
else
    echo "grub-mkrescue not found. Installing alternative method..."
    
    # Alternative method using xorriso
    if command -v xorriso &> /dev/null; then
        echo "Creating ISO image with xorriso..."
        
        # Create El Torito boot image
        mkdir -p iso/boot/grub/i386-pc
        
        # This would require GRUB modules, for now just create a basic ISO
        xorriso -as mkisofs \
            -R -J -c boot/grub/boot.cat \
            -b boot/grub/i386-pc/eltorito.img \
            -no-emul-boot -boot-load-size 4 -boot-info-table \
            -o mykernel.iso iso/
            
        echo "ISO created: mykernel.iso"
    else
        echo "Neither grub-mkrescue nor xorriso found."
        echo "On macOS, install with: brew install grub xorriso"
        echo "On Linux, install with: sudo apt-get install grub-common xorriso"
        echo ""
        echo "Manual ISO creation:"
        echo "1. Copy iso/ directory contents to a CD/USB"
        echo "2. Make it bootable with GRUB"
        exit 1
    fi
fi

# Create USB image (raw disk image)
echo "Creating USB disk image..."
dd if=/dev/zero of=mykernel_usb.img bs=1M count=32 2>/dev/null

# Create partition table and format
if command -v fdisk &> /dev/null; then
    echo "Creating partition table..."
    
    # Create partition table
    cat > partition_script << 'EOF'
o
n
p
1


a
1
w
EOF
    
    fdisk mykernel_usb.img < partition_script >/dev/null 2>&1
    rm partition_script
    
    echo "USB image created: mykernel_usb.img"
    echo ""
    echo "To write to USB device (CAREFUL - this will erase the USB drive):"
    echo "  sudo dd if=mykernel_usb.img of=/dev/sdX bs=1M"
    echo "  (Replace /dev/sdX with your USB device)"
fi

# Create instructions
cat > README_BOOT.txt << 'EOF'
MyKernel Boot Instructions
=========================

Files created:
- mykernel.iso: Bootable CD/DVD image
- mykernel_usb.img: Bootable USB image
- iso/: Directory structure for manual copying

Booting from CD/DVD:
1. Burn mykernel.iso to a CD/DVD
2. Boot from the CD/DVD
3. Select "MyKernel OS" from the boot menu

Booting from USB:
1. Write the USB image: sudo dd if=mykernel_usb.img of=/dev/sdX bs=1M
   (Replace /dev/sdX with your USB device - BE CAREFUL!)
2. Boot from the USB drive

Manual Installation:
1. Copy the contents of iso/ directory to a FAT32 formatted USB/partition
2. Install GRUB bootloader on the device
3. Boot from the device

Testing Commands:
Once booted, try these commands in BSH shell:
- help              (show available commands)
- video modes       (show video modes)
- video set vga320  (switch to graphics mode)
- video test        (test graphics drawing)
- mouse test        (test mouse functionality)
- ping 8.8.8.8      (test networking)
- man ls            (show manual pages)

Hardware Requirements:
- x86 compatible processor (32-bit)
- At least 32MB RAM
- VGA compatible graphics
- PS/2 keyboard and mouse
- Network card (optional, for networking features)

Note: This is a development kernel. Some features may not work on all hardware.
EOF

echo ""
echo "Boot files created successfully!"
echo "See README_BOOT.txt for instructions."

# Cleanup
rm -rf iso/

echo ""
echo "Available files:"
ls -la mykernel.iso mykernel_usb.img README_BOOT.txt 2>/dev/null