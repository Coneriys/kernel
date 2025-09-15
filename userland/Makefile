CC = i686-elf-gcc
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

CFLAGS = -m32 -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -fno-stack-protector
LDFLAGS = -m elf_i386 -Ttext 0x400000

all: hello.bin syscall_test.bin

hello.o: hello.c
	$(CC) $(CFLAGS) -c hello.c -o hello.o

hello.elf: hello.o
	$(LD) $(LDFLAGS) -o hello.elf hello.o

hello.bin: hello.elf
	$(OBJCOPY) -O binary hello.elf hello.bin

syscall_test.o: syscall_test.c
	$(CC) $(CFLAGS) -c syscall_test.c -o syscall_test.o

syscall_test.elf: syscall_test.o
	$(LD) $(LDFLAGS) -o syscall_test.elf syscall_test.o

syscall_test.bin: syscall_test.elf
	$(OBJCOPY) -O binary syscall_test.elf syscall_test.bin

clean:
	rm -f *.o *.elf *.bin

.PHONY: all clean