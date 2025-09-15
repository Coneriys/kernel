#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "interrupts.h"

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

#define KEY_BUFFER_SIZE 128

// Special key codes
#define KEY_UP_ARROW    0x80
#define KEY_DOWN_ARROW  0x81  
#define KEY_LEFT_ARROW  0x82
#define KEY_RIGHT_ARROW 0x83
#define KEY_BACKSPACE   0x08
#define KEY_DELETE      0x7F

void keyboard_init(void);
void keyboard_handler(registers_t regs);
char keyboard_getchar(void);
int keyboard_available(void);
void keyboard_gets(char* buffer, int max_length);
void keyboard_test_debug(void);

#endif