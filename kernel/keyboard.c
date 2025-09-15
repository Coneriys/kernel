#include "../include/keyboard.h"
#include "../include/interrupts.h"

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

static void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static char key_buffer[KEY_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;
static int buffer_count = 0;

static char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	
    '9', '0', '-', '=', KEY_BACKSPACE,	
    '\t',			
    'q', 'w', 'e', 'r',	
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	
    0,			
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	
    '\'', '`',   0,		
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',			
    'm', ',', '.', '/',   0,				
    '*',
    0,	
    ' ',	
    0,	
    0,	
    0, 0, 0, 0, 0, 0, 0, 0,
    0,	
    0,	
    0,	
    0,	
    0,	
    0,	
    '-',
    0,	
    0,
    0,	
    '+',
    0,	
    0,
    0,	
    0,	
    0, 0, 0,
    0,	
    0,	
    0,	
};

static char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	
    '(', ')', '_', '+', KEY_BACKSPACE,	
    '\t',			
    'Q', 'W', 'E', 'R',	
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	
    0,			
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	
    '"', '~',   0,		
    '|', 'Z', 'X', 'C', 'V', 'B', 'N',			
    'M', '<', '>', '?',   0,				
    '*',
    0,	
    ' ',	
    0,	
    0,	
    0, 0, 0, 0, 0, 0, 0, 0,
    0,	
    0,	
    0,	
    0,	
    0,	
    0,	
    '_',
    0,	
    0,
    0,	
    '+',
    0,	
    0,
    0,	
    0,	
    0, 0, 0,
    0,	
    0,	
    0,	
};

static int shift_pressed = 0;
static int caps_lock = 0;
static int escape_sequence = 0;

void keyboard_init(void) {
    terminal_writestring("Initializing PS/2 keyboard...\n");
    
    // Disable devices
    outb(0x64, 0xAD);  // Disable first PS/2 port (keyboard)
    outb(0x64, 0xA7);  // Disable second PS/2 port (mouse)
    
    // Flush output buffer
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    // Set controller configuration byte
    outb(0x64, 0x20);  // Read configuration byte
    while (!(inb(0x64) & 0x01));
    uint8_t config = inb(0x60);
    
    // Enable interrupt on port 1, disable interrupt on port 2
    config |= 0x01;    // Enable keyboard interrupt
    config &= ~0x20;   // Disable mouse interrupt initially
    config &= ~0x10;   // Disable keyboard translation
    
    outb(0x64, 0x60);  // Write configuration byte
    while (inb(0x64) & 0x02);
    outb(0x60, config);
    
    // Perform controller self test
    outb(0x64, 0xAA);
    while (!(inb(0x64) & 0x01));
    uint8_t test = inb(0x60);
    if (test != 0x55) {
        terminal_writestring("PS/2 controller self test failed\n");
        return;
    }
    
    // Enable keyboard port
    outb(0x64, 0xAE);
    
    // Test keyboard port
    outb(0x64, 0xAB);
    while (!(inb(0x64) & 0x01));
    test = inb(0x60);
    if (test != 0x00) {
        terminal_writestring("Keyboard port test failed\n");
    }
    
    // Reset keyboard
    while (inb(0x64) & 0x02);
    outb(0x60, 0xFF);
    while (!(inb(0x64) & 0x01));
    uint8_t ack = inb(0x60);
    
    if (ack == 0xFA) {  // ACK
        while (!(inb(0x64) & 0x01));
        uint8_t result = inb(0x60);
        if (result != 0xAA) {
            terminal_writestring("Keyboard reset failed\n");
        }
    }
    
    // Set scan code set 2
    while (inb(0x64) & 0x02);
    outb(0x60, 0xF0);
    while (!(inb(0x64) & 0x01));
    inb(0x60); // Read ACK
    
    while (inb(0x64) & 0x02);
    outb(0x60, 0x02);
    while (!(inb(0x64) & 0x01));
    inb(0x60); // Read ACK
    
    // Enable keyboard
    while (inb(0x64) & 0x02);
    outb(0x60, 0xF4);
    while (!(inb(0x64) & 0x01));
    inb(0x60); // Read ACK
    
    // Clear buffer
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    // Register interrupt handler
    register_interrupt_handler(33, keyboard_handler);
    
    terminal_writestring("PS/2 keyboard initialized successfully\n");
}

void keyboard_handler(registers_t regs) {
    (void)regs;
    
    // Check if data is available and it's from keyboard (not mouse)
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (!(status & 0x01)) {
        return; // No data available
    }
    
    // Check if data is from mouse (bit 5 of status)
    if (status & 0x20) {
        // This is mouse data, ignore it
        inb(KEYBOARD_DATA_PORT); // Read and discard
        return;
    }
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Handle key releases
    if (scancode & 0x80) {
        scancode &= 0x7F;
        if (scancode == 42 || scancode == 54) {
            shift_pressed = 0;
        }
        return;
    }
    
    // Handle special scancodes for extended keys
    if (scancode == 0xE0) {
        escape_sequence = 1;
        return;
    }
    
    // Handle arrow keys (require E0 prefix)
    if (escape_sequence) {
        escape_sequence = 0;
        char special_key = 0;
        
        switch (scancode) {
            case 0x48: // Up arrow
                special_key = KEY_UP_ARROW;
                break;
            case 0x50: // Down arrow
                special_key = KEY_DOWN_ARROW;
                break;
            case 0x4B: // Left arrow
                special_key = KEY_LEFT_ARROW;
                break;
            case 0x4D: // Right arrow
                special_key = KEY_RIGHT_ARROW;
                break;
            case 0x53: // Delete key
                special_key = KEY_DELETE;
                break;
        }
        
        if (special_key != 0 && buffer_count < KEY_BUFFER_SIZE) {
            key_buffer[buffer_end] = special_key;
            buffer_end = (buffer_end + 1) % KEY_BUFFER_SIZE;
            buffer_count++;
        }
        return;
    }
    
    // Handle shift keys
    if (scancode == 42 || scancode == 54) {
        shift_pressed = 1;
        return;
    }
    
    // Handle caps lock
    if (scancode == 58) {
        caps_lock = !caps_lock;
        return;
    }
    
    char ascii = 0;
    
    // Convert regular scancodes to ASCII
    if (scancode < sizeof(scancode_to_ascii)) {
        if (shift_pressed) {
            ascii = scancode_to_ascii_shift[scancode];
        } else {
            ascii = scancode_to_ascii[scancode];
        }
        
        // Apply caps lock for letters
        if (caps_lock && ascii >= 'a' && ascii <= 'z') {
            ascii = ascii - 'a' + 'A';
        } else if (caps_lock && ascii >= 'A' && ascii <= 'Z') {
            ascii = ascii - 'A' + 'a';
        }
    }
    
    // Add character to buffer
    if (ascii != 0 && buffer_count < KEY_BUFFER_SIZE) {
        key_buffer[buffer_end] = ascii;
        buffer_end = (buffer_end + 1) % KEY_BUFFER_SIZE;
        buffer_count++;
    }
}

char keyboard_getchar(void) {
    if (buffer_count == 0) {
        return 0;
    }
    
    char c = key_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEY_BUFFER_SIZE;
    buffer_count--;
    
    return c;
}

int keyboard_available(void) {
    return buffer_count > 0;
}

void keyboard_gets(char* buffer, int max_length) {
    int index = 0;
    char c;
    
    while (index < max_length - 1) {
        while (!keyboard_available()) {
            asm("hlt");
        }
        
        c = keyboard_getchar();
        
        if (c == '\n') {
            break;
        } else if (c == '\b') {
            if (index > 0) {
                index--;
            }
        } else {
            buffer[index++] = c;
        }
    }
    
    buffer[index] = '\0';
}

void keyboard_test_debug(void) {
    terminal_writestring("PS/2 Keyboard Debug Test\n");
    terminal_writestring("Press keys to see scan codes (ESC to exit)\n\n");
    
    // Clear any pending data
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    while (1) {
        // Check for data
        if (inb(0x64) & 0x01) {
            uint8_t status = inb(0x64);
            uint8_t scancode = inb(0x60);
            
            // Show status and scancode
            terminal_writestring("Status: 0x");
            char hex[3];
            hex[0] = "0123456789ABCDEF"[(status >> 4) & 0xF];
            hex[1] = "0123456789ABCDEF"[status & 0xF];
            hex[2] = '\0';
            terminal_writestring(hex);
            
            terminal_writestring(" Scancode: 0x");
            hex[0] = "0123456789ABCDEF"[(scancode >> 4) & 0xF];
            hex[1] = "0123456789ABCDEF"[scancode & 0xF];
            terminal_writestring(hex);
            
            if (status & 0x20) {
                terminal_writestring(" (Mouse data)");
            } else {
                terminal_writestring(" (Keyboard data)");
            }
            
            terminal_writestring("\n");
            
            // Exit on ESC key
            if (scancode == 0x01) {
                terminal_writestring("Exiting debug mode\n");
                break;
            }
        }
    }
}