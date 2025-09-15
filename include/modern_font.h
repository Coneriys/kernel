#ifndef MODERN_FONT_H
#define MODERN_FONT_H

#include <stdint.h>
#include "metal_api.h"

// Современная система шрифтов для MyKernel
// Основана на принципах SF Pro и Inter - читаемость, чистота, современность

typedef enum {
    FONT_WEIGHT_THIN = 100,
    FONT_WEIGHT_LIGHT = 300,
    FONT_WEIGHT_REGULAR = 400,
    FONT_WEIGHT_MEDIUM = 500,
    FONT_WEIGHT_SEMIBOLD = 600,
    FONT_WEIGHT_BOLD = 700,
    FONT_WEIGHT_HEAVY = 800
} font_weight_t;

typedef enum {
    FONT_STYLE_NORMAL,
    FONT_STYLE_ITALIC
} font_style_t;

typedef struct {
    char name[64];
    font_weight_t weight;
    font_style_t style;
    uint32_t size;
    uint32_t line_height;
    uint32_t letter_spacing;
    uint8_t antialiasing;
    uint8_t subpixel_rendering;
} modern_font_config_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t advance_width;
    int32_t left_bearing;
    int32_t top_bearing;
    uint8_t* bitmap_data;
    uint8_t antialiased;
} modern_glyph_t;

typedef struct {
    char font_name[64];
    uint32_t font_size;
    font_weight_t weight;
    modern_glyph_t glyphs[256]; // ASCII + extended
    uint32_t line_height;
    uint32_t baseline;
    uint32_t cap_height;
    uint32_t x_height;
} modern_font_t;

// Глобальные функции системы шрифтов
int modern_font_init(void);
void modern_font_cleanup(void);

// Управление шрифтами
modern_font_t* modern_font_create_system_default(void);
modern_font_t* modern_font_create_sf_pro(uint32_t size, font_weight_t weight);
modern_font_t* modern_font_create_inter(uint32_t size, font_weight_t weight);
void modern_font_destroy(modern_font_t* font);

// Рендеринг текста
void modern_font_render_text(metal_device_t* device, modern_font_t* font, 
                            const char* text, uint32_t x, uint32_t y, color32_t color);
void modern_font_render_text_advanced(metal_device_t* device, modern_font_t* font,
                                     const char* text, uint32_t x, uint32_t y, 
                                     color32_t color, modern_font_config_t* config);

// Утилиты
uint32_t modern_font_get_text_width(modern_font_t* font, const char* text);
uint32_t modern_font_get_text_height(modern_font_t* font, const char* text);
modern_glyph_t* modern_font_get_glyph(modern_font_t* font, char c);

// Системный шрифт по умолчанию
modern_font_t* modern_font_get_system_font(void);
void modern_font_set_system_font(modern_font_t* font);

// Алгоритмы рендеринга
void modern_font_render_glyph_antialiased(metal_device_t* device, modern_glyph_t* glyph,
                                         uint32_t x, uint32_t y, color32_t color);
void modern_font_render_glyph_subpixel(metal_device_t* device, modern_glyph_t* glyph,
                                      uint32_t x, uint32_t y, color32_t color);

#endif // MODERN_FONT_H
