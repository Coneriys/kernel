#ifndef INTER_FONT_LOADER_H
#define INTER_FONT_LOADER_H

#include <stdint.h>
#include "truetype.h"
#include "metal_api.h"

// Функции для работы с Inter шрифтом
ttf_font_t* create_inter_bitmap_font(void);
void render_inter_style_text(metal_device_t* device, ttf_font_t* font, const char* text,
                           uint32_t x, uint32_t y, uint32_t font_size, color32_t color);
uint32_t get_inter_char_width(char c, uint32_t font_size);
void draw_inter_style_char(metal_device_t* device, char c, uint32_t x, uint32_t y, 
                          uint32_t font_size, color32_t color);

// Вспомогательные функции рисования
void draw_inter_triangle_top(metal_device_t* device, uint32_t x, uint32_t y, 
                            uint32_t width, uint32_t height, uint32_t stroke, color32_t color);
void draw_inter_oval(metal_device_t* device, uint32_t x, uint32_t y, 
                    uint32_t width, uint32_t height, uint32_t stroke, color32_t color);
void draw_anti_aliased_bitmap_char(metal_device_t* device, char c, uint32_t x, uint32_t y, 
                                  uint32_t font_size, color32_t color);

#endif // INTER_FONT_LOADER_H
