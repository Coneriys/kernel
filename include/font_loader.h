// === MyKernel Font Loader Header ===

#ifndef FONT_LOADER_H
#define FONT_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include "truetype.h"
#include "video.h"
#include "metal_api.h"

#define MAX_SYSTEM_FONTS 8

// === Font System Management ===

// Initialize the font system with embedded fonts
int font_loader_init(void);

// Get the main system font (Inter)
ttf_font_t* font_loader_get_system_font(void);

// Load additional fonts from memory
int font_loader_load_from_memory(const uint8_t* font_data, uint32_t size, const char* name);

// Get loaded font by index
ttf_font_t* font_loader_get_font(int index);

// Get number of loaded fonts
int font_loader_get_font_count(void);

// Cleanup font system
void font_loader_cleanup(void);

// === Enhanced Text Rendering ===

// Render text using TrueType font with better character shapes
void font_render_text(metal_device_t* device, ttf_font_t* font, const char* text, 
                      uint32_t x, uint32_t y, uint32_t font_size, color32_t color);

#endif // FONT_LOADER_H