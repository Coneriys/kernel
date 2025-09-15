#include "../include/mouse.h"
// #include "../include/compositor.h" // Removed for rewrite

extern void terminal_writestring(const char* data);

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static mouse_state_t mouse_state;
static uint8_t mouse_packet_buffer[3];
static uint8_t mouse_packet_index = 0;
static uint8_t mouse_initialized = 0;
static int16_t screen_width = MOUSE_SCREEN_WIDTH;
static int16_t screen_height = MOUSE_SCREEN_HEIGHT;

void mouse_wait_input(void) {
    // Wait for input buffer to be empty
    uint32_t timeout = 100000;
    while (timeout-- && (inb(MOUSE_STATUS_PORT) & MOUSE_STATUS_INPUT_FULL));
}

void mouse_wait_output(void) {
    // Wait for output buffer to be full
    uint32_t timeout = 100000;
    while (timeout-- && !(inb(MOUSE_STATUS_PORT) & MOUSE_STATUS_OUTPUT_FULL));
}

void mouse_write_command(uint8_t command) {
    mouse_wait_input();
    outb(MOUSE_COMMAND_PORT, command);
}

void mouse_write_data(uint8_t data) {
    mouse_wait_input();
    outb(MOUSE_DATA_PORT, data);
}

uint8_t mouse_read_data(void) {
    mouse_wait_output();
    return inb(MOUSE_DATA_PORT);
}

void mouse_init(void) {
    // Initialize mouse state
    mouse_state.x = screen_width / 2;
    mouse_state.y = screen_height / 2;
    mouse_state.buttons = 0;
    mouse_state.left_button = 0;
    mouse_state.right_button = 0;
    mouse_state.middle_button = 0;
    
    mouse_packet_index = 0;
    
    terminal_writestring("Initializing PS/2 mouse...\n");
    
    // First, update controller configuration to enable mouse interrupts
    mouse_write_command(0x20);  // Read configuration byte
    uint8_t config = mouse_read_data();
    config |= 0x02;    // Enable mouse interrupt (IRQ 12)
    config &= ~0x20;   // Enable mouse clock
    mouse_write_command(0x60);  // Write configuration byte
    mouse_write_data(config);
    
    // Enable auxiliary device (mouse)
    mouse_write_command(MOUSE_CONTROLLER_CMD_ENABLE_AUX);
    
    // Test auxiliary device
    mouse_write_command(MOUSE_CONTROLLER_CMD_TEST_AUX);
    uint8_t test_result = mouse_read_data();
    if (test_result != 0x00) {
        terminal_writestring("Mouse test failed\n");
        return;
    }
    
    // Reset mouse
    mouse_write_command(MOUSE_CONTROLLER_CMD_WRITE_AUX);
    mouse_write_data(0xFF);  // Reset command
    uint8_t ack = mouse_read_data();
    if (ack == 0xFA) {
        mouse_read_data(); // Read 0xAA (BAT successful)
        mouse_read_data(); // Read device ID
    }
    
    // Set defaults
    mouse_write_command(MOUSE_CONTROLLER_CMD_WRITE_AUX);
    mouse_write_data(MOUSE_CMD_SET_DEFAULTS);
    mouse_read_data(); // Read ACK
    
    // Enable mouse data reporting
    mouse_write_command(MOUSE_CONTROLLER_CMD_WRITE_AUX);
    mouse_write_data(MOUSE_CMD_ENABLE_DATA_REPORTING);
    
    // Read acknowledgment
    ack = mouse_read_data();
    if (ack != 0xFA) {
        terminal_writestring("Mouse enable failed\n");
        return;
    }
    
    // Register interrupt handler (IRQ 12 = interrupt 44)
    register_interrupt_handler(44, mouse_handler);
    terminal_writestring("Mouse interrupt handler registered\n");
    
    mouse_initialized = 1;
    terminal_writestring("PS/2 mouse initialized successfully\n");
}

void mouse_enable(void) {
    if (!mouse_initialized) return;
    
    mouse_write_command(MOUSE_CONTROLLER_CMD_WRITE_AUX);
    mouse_write_data(MOUSE_CMD_ENABLE_DATA_REPORTING);
    mouse_read_data(); // Read ACK
}

void mouse_disable(void) {
    if (!mouse_initialized) return;
    
    mouse_write_command(MOUSE_CONTROLLER_CMD_WRITE_AUX);
    mouse_write_data(MOUSE_CMD_DISABLE_DATA_REPORTING);
    mouse_read_data(); // Read ACK
}

void mouse_handler(registers_t regs) {
    (void)regs; // Unused parameter
    
    // Note: Removed debug output to reduce spam
    
    if (!mouse_initialized) return;
    
    // Check if data is available and it's from the auxiliary device
    uint8_t status = inb(MOUSE_STATUS_PORT);
    if (!(status & MOUSE_STATUS_OUTPUT_FULL) || !(status & MOUSE_STATUS_AUX_DATA)) {
        return;
    }
    
    uint8_t mouse_data = inb(MOUSE_DATA_PORT);
    
    // Build mouse packet (3 bytes)
    mouse_packet_buffer[mouse_packet_index] = mouse_data;
    mouse_packet_index++;
    
    if (mouse_packet_index == 3) {
        // Complete packet received, process it
        mouse_packet_t* packet = (mouse_packet_t*)mouse_packet_buffer;
        
        // Extract button states
        mouse_state.left_button = (packet->flags & MOUSE_PACKET_LEFT_BUTTON) ? 1 : 0;
        mouse_state.right_button = (packet->flags & MOUSE_PACKET_RIGHT_BUTTON) ? 1 : 0;
        mouse_state.middle_button = (packet->flags & MOUSE_PACKET_MIDDLE_BUTTON) ? 1 : 0;
        mouse_state.buttons = packet->flags & 0x07;
        
        // Calculate movement deltas
        int16_t delta_x = packet->x_movement;
        int16_t delta_y = packet->y_movement;
        
        // Handle sign extension
        if (packet->flags & MOUSE_PACKET_X_SIGN) {
            delta_x |= 0xFF00; // Sign extend negative
        }
        if (packet->flags & MOUSE_PACKET_Y_SIGN) {
            delta_y |= 0xFF00; // Sign extend negative
        }
        
        // Update position
        mouse_state.x += delta_x;
        mouse_state.y -= delta_y; // Y is inverted in screen coordinates
        
        // Clamp to screen bounds
        if (mouse_state.x < 0) mouse_state.x = 0;
        if (mouse_state.x >= screen_width) mouse_state.x = screen_width - 1;
        if (mouse_state.y < 0) mouse_state.y = 0;
        if (mouse_state.y >= screen_height) mouse_state.y = screen_height - 1;
        
        // Reset packet index for next packet
        mouse_packet_index = 0;
        
        // GUI integration disabled - will be rewritten
        extern void serial_writestring(const char* str);
        
        // Debug: show mouse movement occasionally
        static int movement_counter = 0;
        movement_counter++;
        if (movement_counter % 60 == 0) {  // Every ~60 movements
            serial_writestring("MOUSE: Movement detected\n");
        }
        
        // Compositor calls removed - will be rewritten
        
        // Handle button events - compositor integration removed
        static uint8_t prev_buttons = 0;
        uint8_t button_changes = mouse_state.buttons ^ prev_buttons;
        
        // Check for button presses/releases
        if (button_changes) {
            serial_writestring("MOUSE: Button event detected\n");
            // GUI integration will be handled by the new GUI2 system
        }
        
        prev_buttons = mouse_state.buttons;
    }
}

mouse_state_t* mouse_get_state(void) {
    return &mouse_state;
}

void mouse_set_position(int16_t x, int16_t y) {
    if (x >= 0 && x < screen_width) {
        mouse_state.x = x;
    }
    if (y >= 0 && y < screen_height) {
        mouse_state.y = y;
    }
}

void mouse_set_screen_bounds(int16_t width, int16_t height) {
    screen_width = width;
    screen_height = height;
    
    // Clamp current position to new bounds
    if (mouse_state.x >= screen_width) mouse_state.x = screen_width - 1;
    if (mouse_state.y >= screen_height) mouse_state.y = screen_height - 1;
}