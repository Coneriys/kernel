#ifndef PS2_H
#define PS2_H

#include <stdint.h>

// PS/2 Controller ports
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

// PS/2 Controller commands
#define PS2_CMD_READ_CONFIG        0x20
#define PS2_CMD_WRITE_CONFIG       0x60
#define PS2_CMD_DISABLE_PORT2      0xA7
#define PS2_CMD_ENABLE_PORT2       0xA8
#define PS2_CMD_TEST_PORT2         0xA9
#define PS2_CMD_TEST_CONTROLLER    0xAA
#define PS2_CMD_TEST_PORT1         0xAB
#define PS2_CMD_DISABLE_PORT1      0xAD
#define PS2_CMD_ENABLE_PORT1       0xAE
#define PS2_CMD_WRITE_PORT2        0xD4

// PS/2 Status register bits
#define PS2_STATUS_OUTPUT_FULL     0x01
#define PS2_STATUS_INPUT_FULL      0x02
#define PS2_STATUS_SYSTEM_FLAG     0x04
#define PS2_STATUS_CMD_DATA        0x08
#define PS2_STATUS_TIMEOUT_ERROR   0x40
#define PS2_STATUS_PARITY_ERROR    0x80
#define PS2_STATUS_AUX_DATA        0x20

// PS/2 Configuration byte bits
#define PS2_CONFIG_PORT1_INT       0x01
#define PS2_CONFIG_PORT2_INT       0x02
#define PS2_CONFIG_PORT1_CLOCK     0x10
#define PS2_CONFIG_PORT2_CLOCK     0x20
#define PS2_CONFIG_PORT1_TRANSLATE 0x40

// Initialize PS/2 controller
void ps2_init(void);
uint8_t ps2_read_data(void);
void ps2_write_data(uint8_t data);
void ps2_write_command(uint8_t cmd);
void ps2_wait_input(void);
void ps2_wait_output(void);

#endif