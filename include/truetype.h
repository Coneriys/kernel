// === MyKernel TrueType Font Engine Header ===

#ifndef TRUETYPE_H
#define TRUETYPE_H

#include <stdint.h>
#include <stddef.h>

// === TTF Structures ===

typedef struct {
    uint16_t major_version;
    uint16_t minor_version;
    uint32_t font_revision;
    uint32_t checksum_adjustment;
    uint32_t magic_number;
    uint16_t flags;
    uint16_t units_per_em;
    int64_t created;
    int64_t modified;
    int16_t x_min, y_min, x_max, y_max;
    uint16_t mac_style;
    uint16_t lowest_rec_ppem;
    int16_t font_direction_hint;
    int16_t index_to_loc_format;
    int16_t glyph_data_format;
} ttf_head_table_t;

typedef struct {
    uint32_t version;
    int16_t ascender;
    int16_t descender;
    int16_t line_gap;
    uint16_t advance_width_max;
    int16_t min_left_side_bearing;
    int16_t min_right_side_bearing;
    int16_t x_max_extent;
    int16_t caret_slope_rise;
    int16_t caret_slope_run;
    int16_t caret_offset;
    int16_t reserved[4];
    int16_t metric_data_format;
    uint16_t number_of_hmetrics;
} ttf_hhea_table_t;

typedef struct {
    uint16_t advance_width;
    int16_t left_side_bearing;
} ttf_long_hor_metric_t;

typedef struct {
    uint16_t version;
    uint16_t num_tables;
    uint16_t encoding_records_offset;
} ttf_cmap_header_t;

typedef struct {
    uint16_t platform_id;
    uint16_t encoding_id;
    uint32_t subtable_offset;
} ttf_cmap_encoding_record_t;

typedef struct {
    uint16_t format;
    uint16_t length;
    uint16_t language;
    uint16_t seg_count_x2;
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;
} ttf_cmap_format4_t;

typedef struct {
    int16_t number_of_contours;
    int16_t x_min, y_min, x_max, y_max;
} ttf_glyph_header_t;

typedef struct {
    uint8_t on_curve;
    int16_t x, y;
} ttf_point_t;

typedef struct {
    uint16_t end_pts_of_contours[64];
    uint16_t instruction_length;
    uint8_t *instructions;
    ttf_point_t *points;
    uint16_t num_points;
    uint16_t num_contours;
} ttf_glyph_t;

typedef struct {
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
} ttf_table_directory_t;

typedef struct {
    const uint8_t *data;
    size_t size;
    
    ttf_head_table_t head;
    ttf_hhea_table_t hhea;
    ttf_long_hor_metric_t *hmtx;
    
    uint32_t *loca;
    const uint8_t *glyf_data;
    
    uint32_t cmap_offset;
    uint16_t *unicode_to_glyph;
    
    uint16_t num_glyphs;
    uint16_t units_per_em;
} ttf_font_t;

typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int advance;
    int left_bearing;
    int top_bearing;
} ttf_bitmap_t;

// === Glyph Metrics ===

typedef struct {
    uint16_t advance_width;
    int16_t left_side_bearing;
} ttf_glyph_metrics_t;

// === Core Functions ===

// Font loading and management
ttf_font_t* ttf_load_font(const uint8_t* font_data, size_t data_size);
void ttf_free_font(ttf_font_t* font);
ttf_bitmap_t* ttf_render_glyph(ttf_font_t* font, uint32_t codepoint, float size);
void ttf_free_bitmap(ttf_bitmap_t* bitmap);
ttf_glyph_t* ttf_load_glyph(ttf_font_t* font, uint16_t glyph_index);
void ttf_free_glyph(ttf_glyph_t* glyph);

// Glyph operations  
uint16_t ttf_get_glyph_index(ttf_font_t* font, uint32_t codepoint);
ttf_glyph_metrics_t ttf_get_glyph_metrics(ttf_font_t* font, uint16_t glyph_index);

// Text measurement
uint32_t ttf_get_text_width(ttf_font_t* font, const char* text, uint32_t font_size);
uint32_t ttf_get_text_height(ttf_font_t* font, uint32_t font_size);

// === Legacy compatibility ===
void ttf_unload_font(ttf_font_t* font);
void ttf_cleanup(void);

#endif // TRUETYPE_H