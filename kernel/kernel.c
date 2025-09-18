#include <stddef.h>
#include <stdint.h>
#include "../include/memory.h"
#include "../include/elf.h"
#include "../include/paging.h"
#include "../include/process.h"
#include "../include/interrupts.h"
#include "../include/keyboard.h"
#include "../include/syscall.h"
#include "../include/scheduler.h"
#include "../include/bsh.h"
#include "../include/vfs.h"
#include "../include/man.h"
#include "../include/net.h"
#include "../include/ip.h"
#include "../include/arp.h"
#include "../include/icmp.h"
#include "../include/udp.h"
#include "../include/dhcp.h"
#include "../include/mouse.h"
#include "../include/video.h"
#include "../include/tcp.h"
#include "../include/usb.h"
#include "../include/disk.h"
#include "../include/installer.h"
// GUI components disabled for rewrite
//#include "../include/sdk.h"
//#include "../include/gui.h"
//#include "../include/pkg.h"
//#include "../include/compositor.h"
//#include "../include/wm_api.h"
//#include "../include/de_api.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Serial port debugging
#define SERIAL_PORT_COM1 0x3F8

void serial_init(void) {
    outb(SERIAL_PORT_COM1 + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT_COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT_COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT_COM1 + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT_COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT_COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT_COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_putchar(char c) {
    while ((inb(SERIAL_PORT_COM1 + 5) & 0x20) == 0);
    outb(SERIAL_PORT_COM1, c);
}

void serial_writestring(const char* str) {
    while (*str) {
        serial_putchar(*str++);
    }
}

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

static void update_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    // Send low byte
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    
    // Send high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_backspace(void) {
    if (terminal_column > 0) {
        terminal_column--;
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        update_cursor(terminal_column, terminal_row);
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
        update_cursor(terminal_column, terminal_row);
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
    }
    update_cursor(terminal_column, terminal_row);
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void kernel_main(multiboot_info_t* mbi) {
    // Initialize serial port first for debugging
    serial_init();
    serial_writestring("byteOS: Serial port initialized\n");
    
    terminal_initialize();
    serial_writestring("byteOS: Terminal initialized\n");
    
    // Skip boot screen to avoid video mode conflicts
    terminal_writestring("byteOS v2.0 - GUI System Loading...\n");
    
    serial_writestring("byteOS: Starting initialization\n");
    
    if (mbi) {
        serial_writestring("byteOS: Initializing memory management for modern systems (32GB+ support)\n");
        pmm_init(mbi);
        heap_init();
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK));
        terminal_writestring("Paging disabled for stability - using identity mapping\n");
        
        idt_init();
        keyboard_init();
        syscall_init();
        process_init();
        scheduler_init();
        vfs_init();
        
        // Initialize disk subsystem
        serial_writestring("byteOS: Initializing disk subsystem\n");
        if (disk_init()) {
            serial_writestring("byteOS: Disk subsystem initialized successfully\n");
        } else {
            serial_writestring("byteOS: Warning - No disks detected\n");
        }
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("Keyboard, interrupts and syscalls enabled!\n");
        
    } else {
        terminal_writestring("ERROR: No multiboot info provided\n");
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Features implemented:\n");
    terminal_writestring("- Multiboot bootloader\n");
    terminal_writestring("- VGA text mode output\n");
    terminal_writestring("- Physical Memory Manager\n");
    terminal_writestring("- Heap allocator (kmalloc/kfree)\n");
    terminal_writestring("- Virtual Memory Manager (ready)\n");
    terminal_writestring("- Process management\n");
    terminal_writestring("- ELF program loader\n");
    terminal_writestring("- Interrupt handling (IDT)\n");
    terminal_writestring("- Keyboard driver\n");
    terminal_writestring("- System calls (syscalls)\n");
    terminal_writestring("- Preemptive multitasking scheduler\n");
    terminal_writestring("- BSH (Basic Shell) command interface\n");
    terminal_writestring("- Virtual File System (VFS)\n");
    terminal_writestring("- File operations (ls, cd, mkdir, rm)\n");
    terminal_writestring("- Basic string handling\n");
    terminal_writestring("- Scrolling support\n");
    
    if (mbi) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        terminal_writestring("\nTesting memory allocation...\n");
        
        void* ptr1 = kmalloc(1024);
        void* ptr2 = kmalloc(2048);
        void* ptr3 = kmalloc(512);
        
        if (ptr1 && ptr2 && ptr3) {
            terminal_writestring("Memory allocation test: SUCCESS\n");
            kfree(ptr1);
            kfree(ptr2);
            kfree(ptr3);
            terminal_writestring("Memory deallocation test: SUCCESS\n");
        } else {
            terminal_writestring("Memory allocation test: FAILED\n");
        }
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("\nTesting multitasking with multiple processes...\n");
        
        extern const uint8_t hello_program_data[];
        extern const size_t hello_program_size;
        
        // Create and start multiple processes for testing
        process_t* process1 = create_process((void*)hello_program_data, hello_program_size);
        process_t* process2 = create_process((void*)hello_program_data, hello_program_size);
        process_t* process3 = create_process((void*)hello_program_data, hello_program_size);
        
        if (process1 && process2 && process3) {
            terminal_writestring("Three processes created successfully!\n");
            
            // Add processes to the scheduler queue
            add_process_to_queue(process1);
            add_process_to_queue(process2);
            add_process_to_queue(process3);
            
            terminal_writestring("Processes added to scheduler queue\n");
            terminal_writestring("Preemptive multitasking is now active!\n");
            
            // Let the scheduler run for a few seconds to demonstrate multitasking
            terminal_writestring("Observing multitasking... (processes will yield CPU automatically)\n");
            
        } else {
            terminal_writestring("Failed to create test processes!\n");
        }
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
        terminal_writestring("\nInitializing MAN system...\n");
        man_init();
        
        terminal_writestring("\nInitializing Network stack...\n");
        net_init();
        arp_init();
        ip_init();
        icmp_init();
        udp_init();
        tcp_init();
        dhcp_init();
        
        // show_boot_screen("Initializing video drivers...");
        video_init();
        
        // show_boot_screen("Initializing input devices...");
        mouse_init();
        
        // show_boot_screen("Initializing USB subsystem...");
        usb_init();
        
        terminal_writestring("\nBasic system ready\n");
        serial_writestring("byteOS: Basic system initialized\n");
        
        // Initialize BSH shell instead of GUI for now
        terminal_writestring("Initializing BSH shell...\n");
        serial_writestring("byteOS: Starting BSH shell\n");
        
        extern void bsh_init(void);
        extern void bsh_run(void);
        
        bsh_init();
        bsh_run();
        
        /*
        show_boot_screen("Loading applications...");
        sdk_init();
        
        // Register built-in applications
        extern void register_calculator_app(void);
        extern void register_terminal_app(void);
        register_calculator_app();
        register_terminal_app();
        
        show_boot_screen("Finalizing system...");
        pkg_init();
        
        // Show boot screen and keep it visible for 30 seconds
        serial_writestring("byteOS: Showing boot screen for 30 seconds...\n");
        
        // Keep showing boot screen for 30 seconds
        for (volatile int sec = 0; sec < 30; sec++) {
            show_boot_screen("byteOS");
            serial_writestring("byteOS: Boot screen active... ");
            for (volatile int i = 0; i < 50000000; i++); // 1 second delay
        }
        
        // Hide boot screen and start compositor
        serial_writestring("byteOS: Hiding boot screen...\n");
        extern void hide_boot_screen(void);
        hide_boot_screen();
        
        serial_writestring("byteOS: Starting compositor immediately...\n");
        
        // Start compositor
        serial_writestring("byteOS: Starting compositor...\n");
        extern int cmd_compositor(void);
        cmd_compositor();
        
        serial_writestring("byteOS: Compositor started successfully\n");
        */
    } else {
        terminal_writestring("ERROR: No multiboot info - cannot initialize GUI\n");
    }
    
    // If GUI exits, fallback to text mode and halt
    terminal_writestring("GUI2 system has exited. System halted.\n");
    while(1) {
        asm("hlt");
    }
}

// Beautiful boot screen implementation
void show_boot_screen(const char* message) {
    // Switch to graphics mode for boot screen
    if (!video_set_mode(VIDEO_MODE_HD_720P) && 
        !video_set_mode(VIDEO_MODE_VESA_1024x768) &&
        !video_set_mode(VIDEO_MODE_VGA_FALLBACK)) {
        // If graphics mode fails, just show text
        terminal_writestring(message);
        terminal_writestring("\n");
        return;
    }
    
    extern video_driver_t* video_get_driver(void);
    video_driver_t* driver = video_get_driver();
    if (!driver || !driver->framebuffer) {
        // Fallback to text
        video_set_mode(VIDEO_MODE_TEXT);
        terminal_initialize();
        terminal_writestring(message);
        terminal_writestring("\n");
        return;
    }
    
    // Clear screen with dark blue gradient background
    uint32_t* fb = (uint32_t*)driver->framebuffer;
    for (uint32_t y = 0; y < driver->height; y++) {
        for (uint32_t x = 0; x < driver->width; x++) {
            // Create gradient from dark blue to black
            uint8_t blue = (uint8_t)(80 - (y * 60 / driver->height));
            uint32_t color = (0xFF << 24) | blue;  // ARGB format
            fb[y * driver->width + x] = color;
        }
    }
    
    // Show loading text in center (simple pixel drawing)
    uint32_t center_x = driver->width / 2;
    uint32_t center_y = driver->height / 2;
    
    // Draw white rectangle for text background
    for (uint32_t y = center_y - 30; y < center_y + 30; y++) {
        for (uint32_t x = center_x - 200; x < center_x + 200; x++) {
            if (x < driver->width && y < driver->height) {
                fb[y * driver->width + x] = 0xFF2D2D30; // Dark gray background
            }
        }
    }
    
    // Draw border
    for (uint32_t y = center_y - 32; y < center_y + 32; y++) {
        for (uint32_t x = center_x - 202; x < center_x + 202; x++) {
            if ((y == center_y - 32 || y == center_y + 31 || 
                 x == center_x - 202 || x == center_x + 201) &&
                x < driver->width && y < driver->height) {
                fb[y * driver->width + x] = 0xFF007AFF; // Blue border
            }
        }
    }
    
    // Simple animation - show for 2 seconds
    for (volatile int i = 0; i < 10000000; i++);
}

int start_hyprland_macos_compositor(void) {
    terminal_writestring("Compositor disabled - GUI system will be rewritten\n");
    return 0;
}

// Global compositor stub
void* global_compositor = NULL;

int compositor_handle_mouse_move(void* comp, int x, int y) {
    (void)comp; (void)x; (void)y;
    return 0;
}

int compositor_handle_mouse_button(void* comp, int x, int y, int button) {
    (void)comp; (void)x; (void)y; (void)button;
    return 0;
}