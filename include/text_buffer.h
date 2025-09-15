#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H

#include "video.h"

void text_buffer_init(void);
int text_buffer_add(const char* text, int x, int y, color32_t color);
void text_buffer_render_all(void);
void text_buffer_clear(void);
void text_buffer_mark_dirty(void);

#endif