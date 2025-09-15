#ifndef FREETYPE_WRAPPER_H
#define FREETYPE_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

// FreeType initialization and cleanup
bool freetype_init(void);
void freetype_cleanup(void);

// Font size management
void freetype_set_size(int pixel_size);

// Text rendering functions
void freetype_render_text(int x, int y, const char* text, uint32_t color);
int freetype_get_text_width(const char* text);

// Compatibility wrappers for existing API
void init_font_system(void);
void render_text(int x, int y, const char* text, uint32_t color);
void render_text_with_background(int x, int y, const char* text, 
                                uint32_t text_color, uint32_t bg_color);
int get_text_width(const char* text);
void draw_char(int x, int y, char c, uint32_t color);

#endif // FREETYPE_WRAPPER_H