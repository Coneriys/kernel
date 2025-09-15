# Simple Master Boot Record for MyKernel
# This creates a bootable disk that loads our kernel

.code16
.section .text

.global _start
_start:
    # Set up segments
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp
    sti

    # Print loading message
    mov $msg_loading, %si
    call print_string

    # Reset disk system
    mov $0x00, %ah
    mov $0x80, %dl      # First hard drive
    int $0x13
    jc disk_error

    # Load kernel from sector 2 onwards
    mov $0x02, %ah      # Read sectors
    mov $32, %al        # Number of sectors to read (adjust as needed)
    mov $0x00, %ch      # Cylinder 0
    mov $0x02, %cl      # Start from sector 2
    mov $0x00, %dh      # Head 0
    mov $0x80, %dl      # Drive 0x80 (first hard drive)
    mov $0x1000, %bx    # Load to 0x1000:0x0000
    mov %bx, %es
    mov $0x0000, %bx
    int $0x13
    jc disk_error

    # Print success message
    mov $msg_loaded, %si
    call print_string

    # Jump to loaded kernel
    ljmp $0x1000, $0x0000

disk_error:
    mov $msg_error, %si
    call print_string
    hlt

print_string:
    pusha
    mov $0x0E, %ah      # BIOS teletype output
print_loop:
    lodsb               # Load byte from SI into AL
    cmp $0, %al         # Check for null terminator
    je print_done
    int $0x10           # BIOS video interrupt
    jmp print_loop
print_done:
    popa
    ret

msg_loading: .asciz "Loading MyKernel...\r\n"
msg_loaded:  .asciz "Kernel loaded. Starting...\r\n"
msg_error:   .asciz "Disk read error!\r\n"

# Fill to 510 bytes and add boot signature
.org 510
.word 0xAA55