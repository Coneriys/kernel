#ifndef EMBEDDED_FONTS_H
#define EMBEDDED_FONTS_H

#include <stdint.h>
#include <stddef.h>

// Inter font family data
extern const uint8_t inter_regular_ttf_data[];
extern const size_t inter_regular_ttf_data_size;

extern const uint8_t inter_bold_ttf_data[];
extern const size_t inter_bold_ttf_data_size;

extern const uint8_t inter_medium_ttf_data[];
extern const size_t inter_medium_ttf_data_size;

// Font loading functions
int load_embedded_fonts(void);
void unload_embedded_fonts(void);

#endif