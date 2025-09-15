CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-gcc

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -nostdlib -lgcc
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

# Build directory
BUILD_DIR = build

# Object files in build directory (GUI components removed, fonts kept, new GUI added)
KERNEL_OBJS = $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/pmm.o $(BUILD_DIR)/heap.o $(BUILD_DIR)/elf.o $(BUILD_DIR)/paging.o $(BUILD_DIR)/paging_asm.o $(BUILD_DIR)/process.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/interrupts_asm.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/bsh.o $(BUILD_DIR)/vfs.o $(BUILD_DIR)/hypr.o $(BUILD_DIR)/man.o $(BUILD_DIR)/net.o $(BUILD_DIR)/ip.o $(BUILD_DIR)/arp.o $(BUILD_DIR)/icmp.o $(BUILD_DIR)/udp.o $(BUILD_DIR)/tcp.o $(BUILD_DIR)/http.o $(BUILD_DIR)/dhcp.o $(BUILD_DIR)/mouse.o $(BUILD_DIR)/video.o $(BUILD_DIR)/pci.o $(BUILD_DIR)/amd_gpu.o $(BUILD_DIR)/usb.o $(BUILD_DIR)/hello_program.o $(BUILD_DIR)/math.o $(BUILD_DIR)/inter_font_data.o $(BUILD_DIR)/ft_kernel.o $(BUILD_DIR)/gui2.o $(BUILD_DIR)/wm2.o

all: $(BUILD_DIR) byteos.bin

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

byteos.bin: $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

# Pattern rules for building object files in build directory

# Assembly files from boot/
$(BUILD_DIR)/boot.o: boot/boot.s | $(BUILD_DIR)
	$(AS) boot/boot.s -o $@

# Assembly files from kernel/
$(BUILD_DIR)/paging_asm.o: kernel/paging_asm.s | $(BUILD_DIR)
	$(AS) kernel/paging_asm.s -o $@

$(BUILD_DIR)/interrupts_asm.o: kernel/interrupts_asm.s | $(BUILD_DIR)
	$(AS) kernel/interrupts_asm.s -o $@

# C files from kernel/
$(BUILD_DIR)/%.o: kernel/%.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# C files from lib/freetype/
$(BUILD_DIR)/ft_kernel.o: lib/freetype/ft_kernel.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# C files from examples/
$(BUILD_DIR)/%.o: examples/%.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# C files from apps/
$(BUILD_DIR)/%.o: apps/%.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Special build rules for hello_program (depends on userland)

tools/embed: tools/embed.c
	gcc tools/embed.c -o tools/embed

userland/syscall_test.elf: userland/syscall_test.c
	cd userland && $(MAKE)

kernel/hello_program.c: userland/syscall_test.elf tools/embed
	./tools/embed userland/syscall_test.elf kernel/hello_program.c hello_program

$(BUILD_DIR)/hello_program.o: kernel/hello_program.c | $(BUILD_DIR)
	$(CC) -c kernel/hello_program.c -o $@ $(CFLAGS)

# Clean target
clean:
	rm -rf $(BUILD_DIR) byteos.bin tools/embed kernel/hello_program.c
	cd userland && $(MAKE) clean

test: byteos.bin
	qemu-system-x86_64 -kernel byteos.bin -netdev user,id=net0 -device rtl8139,netdev=net0 -serial stdio

.PHONY: all clean test
