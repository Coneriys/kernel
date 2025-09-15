#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stddef.h>
#include "interrupts.h"

// PS/2 Mouse ports
#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64
#define MOUSE_COMMAND_PORT 0x64

// Mouse commands
#define MOUSE_CMD_ENABLE_DATA_REPORTING   0xF4
#define MOUSE_CMD_DISABLE_DATA_REPORTING  0xF5
#define MOUSE_CMD_SET_DEFAULTS           0xF6
#define MOUSE_CMD_RESEND                 0xFE
#define MOUSE_CMD_RESET                  0xFF

// Controller commands
#define MOUSE_CONTROLLER_CMD_ENABLE_AUX  0xA8
#define MOUSE_CONTROLLER_CMD_DISABLE_AUX 0xA7
#define MOUSE_CONTROLLER_CMD_TEST_AUX    0xA9
#define MOUSE_CONTROLLER_CMD_WRITE_AUX   0xD4

// Status bits
#define MOUSE_STATUS_OUTPUT_FULL  0x01
#define MOUSE_STATUS_INPUT_FULL   0x02
#define MOUSE_STATUS_AUX_DATA     0x20

// Mouse packet flags
#define MOUSE_PACKET_LEFT_BUTTON    0x01
#define MOUSE_PACKET_RIGHT_BUTTON   0x02
#define MOUSE_PACKET_MIDDLE_BUTTON  0x04
#define MOUSE_PACKET_X_SIGN         0x10
#define MOUSE_PACKET_Y_SIGN         0x20
#define MOUSE_PACKET_X_OVERFLOW     0x40
#define MOUSE_PACKET_Y_OVERFLOW     0x80

// Mouse state structure
typedef struct {
    int16_t x;              // X position
    int16_t y;              // Y position
    uint8_t buttons;        // Button state
    uint8_t left_button;    // Left button pressed
    uint8_t right_button;   // Right button pressed
    uint8_t middle_button;  // Middle button pressed
} mouse_state_t;

// Mouse packet structure (3 bytes)
typedef struct {
    uint8_t flags;          // Button and sign flags
    uint8_t x_movement;     // X movement delta
    uint8_t y_movement;     // Y movement delta
} mouse_packet_t;

// Screen dimensions for cursor bounds (updated for HD graphics)
#define MOUSE_SCREEN_WIDTH  1920
#define MOUSE_SCREEN_HEIGHT 1080

// Mouse initialization and control
void mouse_init(void);
void mouse_handler(registers_t regs);
void mouse_enable(void);
void mouse_disable(void);

// Mouse state access
mouse_state_t* mouse_get_state(void);
void mouse_set_position(int16_t x, int16_t y);
void mouse_set_screen_bounds(int16_t width, int16_t height);

// Helper functions
void mouse_wait_input(void);
void mouse_wait_output(void);
void mouse_write_command(uint8_t command);
void mouse_write_data(uint8_t data);
uint8_t mouse_read_data(void);

#endif