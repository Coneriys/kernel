// === Complete SF Pro Font Implementation for MyKernel ===
#include "../include/modern_font.h"
#include "../include/memory.h"
#include "../include/metal_api.h"
#include <stddef.h>

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);
extern void serial_writestring(const char* str);
extern size_t strlen(const char* str);

static modern_font_t* system_font = NULL;
static int font_system_initialized = 0;

// === SF Pro Design Constants ===
#define SF_PRO_CAP_HEIGHT_RATIO 0.72f      // Capital letter height ratio
#define SF_PRO_X_HEIGHT_RATIO 0.52f        // Lowercase letter height ratio
#define SF_PRO_ASCENDER_RATIO 0.85f        // Ascender height ratio
#define SF_PRO_DESCENDER_RATIO 0.15f       // Descender depth ratio
#define SF_PRO_LETTER_SPACING 0.02f        // Letter spacing ratio
#define SF_PRO_OPTICAL_SIZE_THRESHOLD 20   // Switch between Text and Display

// === Mathematical Functions ===
static float sqrtf(float x) {
    if (x < 0) return 0;
    if (x == 0) return 0;
    float guess = x / 2.0f;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}

static int abs(int x) {
    return x < 0 ? -x : x;
}

static float min(float a, float b) {
    return a < b ? a : b;
}

static float max(float a, float b) {
    return a > b ? a : b;
}

static char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

// === Antialiasing Functions ===
static uint8_t blend_alpha(uint8_t bg, uint8_t fg, uint8_t alpha) {
    return (bg * (255 - alpha) + fg * alpha) / 255;
}

static uint8_t sf_antialiasing(float distance) {
    if (distance <= 0.0f) return 255;
    if (distance >= 1.0f) return 0;
    // SF Pro uses gamma-corrected antialiasing for sharper edges
    float alpha = 1.0f - distance;
    alpha = alpha * alpha; // Gamma correction
    return (uint8_t)(alpha * 255.0f);
}

// === Color Blending ===
static color32_t blend_color(color32_t bg, color32_t fg, uint8_t alpha) {
    color32_t result;
    result.r = blend_alpha(bg.r, fg.r, alpha);
    result.g = blend_alpha(bg.g, fg.g, alpha);
    result.b = blend_alpha(bg.b, fg.b, alpha);
    result.a = 255;
    return result;
}

// === SF Pro Stroke Width Calculation ===
static uint32_t sf_pro_stroke_width(uint32_t size, font_weight_t weight) {
    float base_ratio;
    
    // SF Pro weight ratios based on Apple's specifications
    switch (weight) {
        case FONT_WEIGHT_THIN:      base_ratio = 0.04f; break;
        case FONT_WEIGHT_LIGHT:     base_ratio = 0.06f; break;
        case FONT_WEIGHT_REGULAR:   base_ratio = 0.08f; break;
        case FONT_WEIGHT_MEDIUM:    base_ratio = 0.10f; break;
        case FONT_WEIGHT_SEMIBOLD:  base_ratio = 0.12f; break;
        case FONT_WEIGHT_BOLD:      base_ratio = 0.15f; break;
        case FONT_WEIGHT_HEAVY:     base_ratio = 0.18f; break;
        default:                    base_ratio = 0.08f; break;
    }
    
    // Adjust for optical size (Text vs Display)
    if (size < SF_PRO_OPTICAL_SIZE_THRESHOLD) {
        base_ratio *= 1.1f; // Slightly thicker for smaller sizes
    }
    
    uint32_t stroke = (uint32_t)(size * base_ratio);
    return stroke < 1 ? 1 : stroke;
}

// === Drawing Primitives ===
static void draw_pixel_aa(uint8_t* bitmap, uint32_t w, uint32_t h, float x, float y, uint8_t value) {
    int ix = (int)x;
    int iy = (int)y;
    
    if (ix < 0 || iy < 0 || ix >= (int)w - 1 || iy >= (int)h - 1) return;
    
    float fx = x - ix;
    float fy = y - iy;
    
    // Bilinear interpolation for subpixel accuracy
    bitmap[iy * w + ix] = max(bitmap[iy * w + ix], (uint8_t)(value * (1 - fx) * (1 - fy)));
    bitmap[iy * w + ix + 1] = max(bitmap[iy * w + ix + 1], (uint8_t)(value * fx * (1 - fy)));
    bitmap[(iy + 1) * w + ix] = max(bitmap[(iy + 1) * w + ix], (uint8_t)(value * (1 - fx) * fy));
    bitmap[(iy + 1) * w + ix + 1] = max(bitmap[(iy + 1) * w + ix + 1], (uint8_t)(value * fx * fy));
}

static void draw_line(uint8_t* bitmap, uint32_t w, uint32_t h, 
                     float x1, float y1, float x2, float y2, float stroke) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    
    if (length == 0) return;
    
    // Normalize direction
    dx /= length;
    dy /= length;
    
    // Draw along the line
    for (float t = 0; t <= length; t += 0.5f) {
        float x = x1 + dx * t;
        float y = y1 + dy * t;
        
        // Draw circular brush at this point
        for (float sy = -stroke/2; sy <= stroke/2; sy += 0.5f) {
            for (float sx = -stroke/2; sx <= stroke/2; sx += 0.5f) {
                float dist = sqrtf(sx * sx + sy * sy);
                if (dist <= stroke/2) {
                    uint8_t alpha = sf_antialiasing(dist - stroke/2 + 1);
                    draw_pixel_aa(bitmap, w, h, x + sx, y + sy, alpha);
                }
            }
        }
    }
}

static void draw_arc(uint8_t* bitmap, uint32_t w, uint32_t h,
                    float cx, float cy, float radius, float stroke,
                    float start_angle, float end_angle) {
    float angle_step = 0.05f; // Radians
    
    for (float angle = start_angle; angle <= end_angle; angle += angle_step) {
        float x = cx + radius * cosf(angle);
        float y = cy + radius * sinf(angle);
        
        // Draw point with stroke width
        for (float sy = -stroke/2; sy <= stroke/2; sy += 0.5f) {
            for (float sx = -stroke/2; sx <= stroke/2; sx += 0.5f) {
                float dist = sqrtf(sx * sx + sy * sy);
                if (dist <= stroke/2) {
                    uint8_t alpha = sf_antialiasing(dist - stroke/2 + 1);
                    draw_pixel_aa(bitmap, w, h, x + sx, y + sy, alpha);
                }
            }
        }
    }
}

static void draw_circle(uint8_t* bitmap, uint32_t w, uint32_t h,
                       float cx, float cy, float radius, float stroke) {
    draw_arc(bitmap, w, h, cx, cy, radius, stroke, 0, 6.28318f); // 2*PI
}

// Simple trigonometric approximations
static float sinf(float x) {
    // Normalize to [-pi, pi]
    while (x > 3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    
    // Taylor series approximation
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f + x2 * x2 / 120.0f);
}

static float cosf(float x) {
    return sinf(x + 1.5708f); // cos(x) = sin(x + pi/2)
}

// === SF Pro Character Rendering Functions ===

// Letter A
static void sf_pro_render_a(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital A - triangular with crossbar
        float apex_x = w / 2.0f;
        float apex_y = baseline;
        float base_y = baseline + cap_height;
        float base_left = w * 0.1f;
        float base_right = w * 0.9f;
        
        // Left stroke
        draw_line(bitmap, w, h, apex_x, apex_y, base_left, base_y, stroke);
        
        // Right stroke
        draw_line(bitmap, w, h, apex_x, apex_y, base_right, base_y, stroke);
        
        // Crossbar
        float crossbar_y = baseline + cap_height * 0.45f;
        float crossbar_left = w * 0.25f;
        float crossbar_right = w * 0.75f;
        draw_line(bitmap, w, h, crossbar_left, crossbar_y, crossbar_right, crossbar_y, stroke);
    } else {
        // Lowercase a - circular bowl with stem
        float bowl_center_x = w * 0.45f;
        float bowl_center_y = baseline + x_height * 0.5f;
        float bowl_radius = x_height * 0.45f;
        
        // Bowl (incomplete circle)
        draw_arc(bitmap, w, h, bowl_center_x, bowl_center_y, bowl_radius, stroke, 0.3f, 5.8f);
        
        // Stem
        float stem_x = w * 0.85f;
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height, stroke);
        
        // Connection
        draw_line(bitmap, w, h, bowl_center_x + bowl_radius * 0.7f, bowl_center_y, stem_x, bowl_center_y, stroke);
    }
}

// Letter B
static void sf_pro_render_b(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital B - vertical line with two bumps
        float stem_x = w * 0.15f;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
        
        // Top curve
        float top_radius = cap_height * 0.25f;
        float top_center_x = stem_x + top_radius;
        float top_center_y = baseline + top_radius;
        draw_arc(bitmap, w, h, top_center_x, top_center_y, top_radius, stroke, -1.57f, 1.57f);
        
        // Bottom curve (slightly larger)
        float bottom_radius = cap_height * 0.3f;
        float bottom_center_x = stem_x + bottom_radius;
        float bottom_center_y = baseline + cap_height - bottom_radius;
        draw_arc(bitmap, w, h, bottom_center_x, bottom_center_y, bottom_radius, stroke, -1.57f, 1.57f);
        
        // Middle connection
        float mid_y = baseline + cap_height * 0.5f;
        draw_line(bitmap, w, h, stem_x, mid_y, stem_x + w * 0.5f, mid_y, stroke);
    } else {
        // Lowercase b - tall stem with bowl
        float stem_x = w * 0.15f;
        float ascender_height = h * SF_PRO_ASCENDER_RATIO;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline * 0.5f, stem_x, baseline + x_height, stroke);
        
        // Bowl
        float bowl_center_x = w * 0.55f;
        float bowl_center_y = baseline + x_height * 0.5f;
        float bowl_radius = x_height * 0.45f;
        draw_circle(bitmap, w, h, bowl_center_x, bowl_center_y, bowl_radius, stroke);
        
        // Connection
        draw_line(bitmap, w, h, stem_x, bowl_center_y, bowl_center_x - bowl_radius, bowl_center_y, stroke * 0.8f);
    }
}

// Letter C
static void sf_pro_render_c(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    float center_x = w * 0.5f;
    float center_y = baseline + height * 0.5f;
    float radius = height * 0.45f;
    
    // Draw arc (not complete circle)
    draw_arc(bitmap, w, h, center_x, center_y, radius, stroke, 0.5f, 5.78f);
}

// Letter D
static void sf_pro_render_d(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital D - vertical line with arc
        float stem_x = w * 0.15f;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
        
        // Arc
        float arc_center_x = stem_x;
        float arc_center_y = baseline + cap_height * 0.5f;
        float arc_radius = cap_height * 0.5f;
        draw_arc(bitmap, w, h, arc_center_x, arc_center_y, arc_radius, stroke, -1.57f, 1.57f);
        
        // Top and bottom connections
        draw_line(bitmap, w, h, stem_x, baseline, stem_x + w * 0.3f, baseline, stroke);
        draw_line(bitmap, w, h, stem_x, baseline + cap_height, stem_x + w * 0.3f, baseline + cap_height, stroke);
    } else {
        // Lowercase d - bowl with tall stem
        float stem_x = w * 0.85f;
        float ascender_height = h * SF_PRO_ASCENDER_RATIO;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline * 0.5f, stem_x, baseline + x_height, stroke);
        
        // Bowl
        float bowl_center_x = w * 0.45f;
        float bowl_center_y = baseline + x_height * 0.5f;
        float bowl_radius = x_height * 0.45f;
        draw_circle(bitmap, w, h, bowl_center_x, bowl_center_y, bowl_radius, stroke);
        
        // Connection
        draw_line(bitmap, w, h, bowl_center_x + bowl_radius, bowl_center_y, stem_x, bowl_center_y, stroke * 0.8f);
    }
}

// Letter E
static void sf_pro_render_e(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital E
        float stem_x = w * 0.15f;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
        
        // Top horizontal
        draw_line(bitmap, w, h, stem_x, baseline, w * 0.85f, baseline, stroke);
        
        // Middle horizontal (slightly shorter)
        float mid_y = baseline + cap_height * 0.5f;
        draw_line(bitmap, w, h, stem_x, mid_y, w * 0.75f, mid_y, stroke);
        
        // Bottom horizontal
        draw_line(bitmap, w, h, stem_x, baseline + cap_height, w * 0.85f, baseline + cap_height, stroke);
    } else {
        // Lowercase e - circle with horizontal bar
        float center_x = w * 0.5f;
        float center_y = baseline + x_height * 0.5f;
        float radius = x_height * 0.45f;
        
        // Draw incomplete circle
        draw_arc(bitmap, w, h, center_x, center_y, radius, stroke, -0.3f, 5.5f);
        
        // Horizontal bar
        draw_line(bitmap, w, h, center_x - radius * 0.8f, center_y, center_x + radius * 0.6f, center_y, stroke);
    }
}

// Continue with more letters...
// Letter F
static void sf_pro_render_f(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital F
        float stem_x = w * 0.15f;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
        
        // Top horizontal
        draw_line(bitmap, w, h, stem_x, baseline, w * 0.85f, baseline, stroke);
        
        // Middle horizontal (slightly shorter)
        float mid_y = baseline + cap_height * 0.45f;
        draw_line(bitmap, w, h, stem_x, mid_y, w * 0.75f, mid_y, stroke);
    } else {
        // Lowercase f - with curve at top
        float stem_x = w * 0.5f;
        float ascender_height = h * SF_PRO_ASCENDER_RATIO;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + ascender_height * 0.8f, stroke);
        
        // Top curve
        float curve_radius = w * 0.3f;
        draw_arc(bitmap, w, h, stem_x + curve_radius, baseline + ascender_height * 0.8f - curve_radius, 
                 curve_radius, stroke, 3.14f, 4.71f);
        
        // Crossbar
        float crossbar_y = baseline + x_height * 0.9f;
        draw_line(bitmap, w, h, w * 0.15f, crossbar_y, w * 0.85f, crossbar_y, stroke);
    }
}

// Letter G
static void sf_pro_render_g(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital G - C with horizontal bar
        float center_x = w * 0.5f;
        float center_y = baseline + cap_height * 0.5f;
        float radius = cap_height * 0.45f;
        
        // Draw arc
        draw_arc(bitmap, w, h, center_x, center_y, radius, stroke, 0.3f, 5.98f);
        
        // Horizontal bar
        float bar_y = center_y;
        draw_line(bitmap, w, h, center_x, bar_y, center_x + radius * 0.8f, bar_y, stroke);
        
        // Vertical connection
        draw_line(bitmap, w, h, center_x + radius * 0.8f, bar_y, center_x + radius * 0.8f, bar_y + radius * 0.5f, stroke);
    } else {
        // Lowercase g - with descender
        float center_x = w * 0.5f;
        float center_y = baseline + x_height * 0.5f;
        float radius = x_height * 0.45f;
        
        // Upper bowl
        draw_circle(bitmap, w, h, center_x, center_y, radius, stroke);
        
        // Stem
        float stem_x = center_x + radius * 0.7f;
        float descender_depth = h * SF_PRO_DESCENDER_RATIO;
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height + descender_depth * 0.7f, stroke);
        
        // Bottom curve
        draw_arc(bitmap, w, h, stem_x - radius * 0.5f, baseline + x_height + descender_depth * 0.7f,
                 radius * 0.5f, stroke, 0, 3.14f);
    }
}

// Letter H
static void sf_pro_render_h(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital H
        float left_x = w * 0.15f;
        float right_x = w * 0.85f;
        
        // Left vertical
        draw_line(bitmap, w, h, left_x, baseline, left_x, baseline + cap_height, stroke);
        
        // Right vertical
        draw_line(bitmap, w, h, right_x, baseline, right_x, baseline + cap_height, stroke);
        
        // Horizontal crossbar
        float crossbar_y = baseline + cap_height * 0.5f;
        draw_line(bitmap, w, h, left_x, crossbar_y, right_x, crossbar_y, stroke);
    } else {
        // Lowercase h
        float stem_x = w * 0.15f;
        float ascender_height = h * SF_PRO_ASCENDER_RATIO;
        
        // Left vertical (with ascender)
        draw_line(bitmap, w, h, stem_x, baseline * 0.5f, stem_x, baseline + x_height, stroke);
        
        // Arch
        float arch_start_y = baseline + x_height * 0.6f;
        float arch_end_x = w * 0.85f;
        
        // Arch curve
        float arch_radius = (arch_end_x - stem_x) * 0.5f;
        draw_arc(bitmap, w, h, stem_x + arch_radius, arch_start_y, arch_radius, stroke, 3.14f, 0);
        
        // Right vertical
        draw_line(bitmap, w, h, arch_end_x, arch_start_y, arch_end_x, baseline + x_height, stroke);
    }
}

// Letter I
static void sf_pro_render_i(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    
    if (is_uppercase) {
        // Capital I - simple vertical line with serifs
        draw_line(bitmap, w, h, center_x, baseline, center_x, baseline + cap_height, stroke);
        
        // Optional: Add small serifs for SF Pro style
        float serif_width = w * 0.3f;
        draw_line(bitmap, w, h, center_x - serif_width, baseline, center_x + serif_width, baseline, stroke * 0.8f);
        draw_line(bitmap, w, h, center_x - serif_width, baseline + cap_height, center_x + serif_width, baseline + cap_height, stroke * 0.8f);
    } else {
        // Lowercase i - with dot
        draw_line(bitmap, w, h, center_x, baseline, center_x, baseline + x_height, stroke);
        
        // Dot
        float dot_y = baseline + x_height + x_height * 0.3f;
        float dot_radius = stroke * 0.6f;
        draw_circle(bitmap, w, h, center_x, dot_y, dot_radius, dot_radius * 2);
    }
}

// Letter J
static void sf_pro_render_j(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital J - vertical with bottom curve
        float stem_x = w * 0.7f;
        float curve_radius = w * 0.35f;
        
        // Vertical line
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height * 0.7f, stroke);
        
        // Bottom curve
        draw_arc(bitmap, w, h, stem_x - curve_radius, baseline + cap_height - curve_radius,
                 curve_radius, stroke, 0, 1.57f);
    } else {
        // Lowercase j - with dot and descender
        float stem_x = w * 0.5f;
        float descender_depth = h * SF_PRO_DESCENDER_RATIO;
        
        // Vertical line
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height + descender_depth * 0.5f, stroke);
        
        // Bottom curve
        float curve_radius = w * 0.4f;
        draw_arc(bitmap, w, h, stem_x - curve_radius, baseline + x_height + descender_depth * 0.5f,
                 curve_radius, stroke, 0, 1.57f);
        
        // Dot
        float dot_y = baseline + x_height + x_height * 0.3f;
        float dot_radius = stroke * 0.6f;
        draw_circle(bitmap, w, h, stem_x, dot_y, dot_radius, dot_radius * 2);
    }
}

// Letter K
static void sf_pro_render_k(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float stem_x = w * 0.15f;
    float height = is_uppercase ? cap_height : x_height;
    
    // Vertical stem
    if (is_uppercase) {
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
    } else {
        // Lowercase k has ascender
        float ascender_height = h * SF_PRO_ASCENDER_RATIO;
        draw_line(bitmap, w, h, stem_x, baseline * 0.5f, stem_x, baseline + x_height, stroke);
    }
    
    // Upper diagonal
    float junction_y = baseline + height * 0.55f;
    draw_line(bitmap, w, h, stem_x + stroke * 0.5f, junction_y, w * 0.85f, baseline, stroke);
    
    // Lower diagonal
    draw_line(bitmap, w, h, stem_x + stroke * 0.5f, junction_y, w * 0.85f, baseline + height, stroke);
}

// Letter L
static void sf_pro_render_l(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float stem_x = w * 0.15f;
    
    if (is_uppercase) {
        // Capital L
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
        draw_line(bitmap, w, h, stem_x, baseline + cap_height, w * 0.85f, baseline + cap_height, stroke);
    } else {
        // Lowercase l - simple vertical line with ascender
        float center_x = w * 0.5f;
        float ascender_height = h * SF_PRO_ASCENDER_RATIO;
        draw_line(bitmap, w, h, center_x, baseline * 0.5f, center_x, baseline + x_height, stroke);
    }
}

// Letter M
static void sf_pro_render_m(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital M
        float left_x = w * 0.1f;
        float right_x = w * 0.9f;
        float center_x = w * 0.5f;
        float top_y = baseline;
        float bottom_y = baseline + cap_height;
        
        // Left vertical
        draw_line(bitmap, w, h, left_x, top_y, left_x, bottom_y, stroke);
        
        // Left diagonal
        draw_line(bitmap, w, h, left_x, top_y, center_x, bottom_y * 0.6f, stroke);
        
        // Right diagonal
        draw_line(bitmap, w, h, center_x, bottom_y * 0.6f, right_x, top_y, stroke);
        
        // Right vertical
        draw_line(bitmap, w, h, right_x, top_y, right_x, bottom_y, stroke);
    } else {
        // Lowercase m - with two humps
        float stem_x = w * 0.1f;
        float hump1_x = w * 0.35f;
        float hump2_x = w * 0.65f;
        float end_x = w * 0.9f;
        
        // First stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height, stroke);
        
        // First hump
        float arch_radius = (hump1_x - stem_x);
        draw_arc(bitmap, w, h, stem_x + arch_radius, baseline + x_height * 0.7f, arch_radius, stroke, 3.14f, 0);
        draw_line(bitmap, w, h, hump1_x, baseline + x_height * 0.7f, hump1_x, baseline + x_height, stroke);
        
        // Second hump
        draw_arc(bitmap, w, h, hump1_x + arch_radius, baseline + x_height * 0.7f, arch_radius, stroke, 3.14f, 0);
        draw_line(bitmap, w, h, hump2_x, baseline + x_height * 0.7f, hump2_x, baseline + x_height, stroke);
        
        // Third stem
        draw_arc(bitmap, w, h, hump2_x + arch_radius * 0.8f, baseline + x_height * 0.7f, arch_radius * 0.8f, stroke, 3.14f, 0);
        draw_line(bitmap, w, h, end_x, baseline + x_height * 0.7f, end_x, baseline + x_height, stroke);
    }
}

// Letter N
static void sf_pro_render_n(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital N
        float left_x = w * 0.15f;
        float right_x = w * 0.85f;
        
        // Left vertical
        draw_line(bitmap, w, h, left_x, baseline, left_x, baseline + cap_height, stroke);
        
        // Diagonal
        draw_line(bitmap, w, h, left_x, baseline + cap_height, right_x, baseline, stroke);
        
        // Right vertical
        draw_line(bitmap, w, h, right_x, baseline, right_x, baseline + cap_height, stroke);
    } else {
        // Lowercase n - with hump
        float stem_x = w * 0.15f;
        float end_x = w * 0.85f;
        
        // Left stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height, stroke);
        
        // Arch
        float arch_radius = (end_x - stem_x) * 0.5f;
        draw_arc(bitmap, w, h, stem_x + arch_radius, baseline + x_height * 0.7f, arch_radius, stroke, 3.14f, 0);
        
        // Right stem
        draw_line(bitmap, w, h, end_x, baseline + x_height * 0.7f, end_x, baseline + x_height, stroke);
    }
}

// Letter O
static void sf_pro_render_o(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    float center_x = w * 0.5f;
    float center_y = baseline + height * 0.5f;
    float radius = height * 0.48f;
    
    // Perfect circle for O/o
    draw_circle(bitmap, w, h, center_x, center_y, radius, stroke);
}

// Letter P
static void sf_pro_render_p(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital P
        float stem_x = w * 0.15f;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
        
        // Bowl
        float bowl_radius = cap_height * 0.3f;
        float bowl_center_x = stem_x + bowl_radius;
        float bowl_center_y = baseline + bowl_radius;
        
        // Top horizontal
        draw_line(bitmap, w, h, stem_x, baseline, bowl_center_x, baseline, stroke);
        
        // Bowl arc
        draw_arc(bitmap, w, h, bowl_center_x, bowl_center_y, bowl_radius, stroke, -1.57f, 1.57f);
        
        // Middle horizontal
        draw_line(bitmap, w, h, stem_x, baseline + bowl_radius * 2, bowl_center_x, baseline + bowl_radius * 2, stroke);
    } else {
        // Lowercase p - with descender
        float stem_x = w * 0.15f;
        float descender_depth = h * SF_PRO_DESCENDER_RATIO;
        
        // Vertical stem with descender
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height + descender_depth, stroke);
        
        // Bowl
        float bowl_center_x = w * 0.55f;
        float bowl_center_y = baseline + x_height * 0.5f;
        float bowl_radius = x_height * 0.45f;
        draw_circle(bitmap, w, h, bowl_center_x, bowl_center_y, bowl_radius, stroke);
        
        // Connection
        draw_line(bitmap, w, h, stem_x, bowl_center_y, bowl_center_x - bowl_radius, bowl_center_y, stroke * 0.8f);
    }
}

// Letter Q
static void sf_pro_render_q(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital Q - O with tail
        float center_x = w * 0.5f;
        float center_y = baseline + cap_height * 0.5f;
        float radius = cap_height * 0.48f;
        
        // Circle
        draw_circle(bitmap, w, h, center_x, center_y, radius, stroke);
        
        // Tail
        float tail_start_x = center_x + radius * 0.6f;
        float tail_start_y = center_y + radius * 0.6f;
        float tail_end_x = w * 0.85f;
        float tail_end_y = baseline + cap_height;
        draw_line(bitmap, w, h, tail_start_x, tail_start_y, tail_end_x, tail_end_y, stroke);
    } else {
        // Lowercase q - with descender
        float bowl_center_x = w * 0.45f;
        float bowl_center_y = baseline + x_height * 0.5f;
        float bowl_radius = x_height * 0.45f;
        float stem_x = w * 0.85f;
        float descender_depth = h * SF_PRO_DESCENDER_RATIO;
        
        // Bowl
        draw_circle(bitmap, w, h, bowl_center_x, bowl_center_y, bowl_radius, stroke);
        
        // Stem with descender
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height + descender_depth, stroke);
        
        // Connection
        draw_line(bitmap, w, h, bowl_center_x + bowl_radius, bowl_center_y, stem_x, bowl_center_y, stroke * 0.8f);
    }
}

// Letter R
static void sf_pro_render_r(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    if (is_uppercase) {
        // Capital R - P with leg
        float stem_x = w * 0.15f;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + cap_height, stroke);
        
        // Bowl
        float bowl_radius = cap_height * 0.25f;
        float bowl_center_x = stem_x + bowl_radius;
        float bowl_center_y = baseline + bowl_radius;
        
        // Top horizontal
        draw_line(bitmap, w, h, stem_x, baseline, bowl_center_x, baseline, stroke);
        
        // Bowl arc
        draw_arc(bitmap, w, h, bowl_center_x, bowl_center_y, bowl_radius, stroke, -1.57f, 1.57f);
        
        // Middle horizontal
        float mid_y = baseline + bowl_radius * 2;
        draw_line(bitmap, w, h, stem_x, mid_y, bowl_center_x, mid_y, stroke);
        
        // Diagonal leg
        draw_line(bitmap, w, h, stem_x + stroke, mid_y, w * 0.85f, baseline + cap_height, stroke);
    } else {
        // Lowercase r - with short arm
        float stem_x = w * 0.2f;
        
        // Vertical stem
        draw_line(bitmap, w, h, stem_x, baseline, stem_x, baseline + x_height, stroke);
        
        // Arm
        float arm_start_y = baseline + x_height * 0.7f;
        float arm_end_x = w * 0.8f;
        float arm_radius = (arm_end_x - stem_x) * 0.5f;
        draw_arc(bitmap, w, h, stem_x + arm_radius, arm_start_y, arm_radius * 0.7f, stroke, 3.14f, 4.5f);
    }
}

// Letter S
static void sf_pro_render_s(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    float center_x = w * 0.5f;
    
    // Top curve
    float top_radius = height * 0.25f;
    float top_center_x = center_x;
    float top_center_y = baseline + top_radius;
    draw_arc(bitmap, w, h, top_center_x, top_center_y, top_radius, stroke, 0.5f, 3.64f);
    
    // Middle diagonal (subtle)
    float mid_start_x = center_x - top_radius * 0.7f;
    float mid_start_y = baseline + height * 0.35f;
    float mid_end_x = center_x + top_radius * 0.7f;
    float mid_end_y = baseline + height * 0.65f;
    draw_line(bitmap, w, h, mid_start_x, mid_start_y, mid_end_x, mid_end_y, stroke);
    
    // Bottom curve
    float bottom_radius = height * 0.25f;
    float bottom_center_x = center_x;
    float bottom_center_y = baseline + height - bottom_radius;
    draw_arc(bitmap, w, h, bottom_center_x, bottom_center_y, bottom_radius, stroke, -0.5f, 2.64f);
}

// Letter T
static void sf_pro_render_t(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    
    if (is_uppercase) {
        // Capital T
        // Top horizontal
        draw_line(bitmap, w, h, w * 0.1f, baseline, w * 0.9f, baseline, stroke);
        
        // Vertical stem
        draw_line(bitmap, w, h, center_x, baseline, center_x, baseline + cap_height, stroke);
    } else {
        // Lowercase t - with crossbar
        float ascender_partial = x_height * 1.3f;
        
        // Vertical stem
        draw_line(bitmap, w, h, center_x, baseline, center_x, baseline + ascender_partial, stroke);
        
        // Crossbar
        float crossbar_y = baseline + x_height * 0.85f;
        draw_line(bitmap, w, h, w * 0.2f, crossbar_y, w * 0.8f, crossbar_y, stroke);
    }
}

// Letter U
static void sf_pro_render_u(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    float left_x = w * 0.15f;
    float right_x = w * 0.85f;
    
    // Left vertical
    draw_line(bitmap, w, h, left_x, baseline, left_x, baseline + height * 0.6f, stroke);
    
    // Right vertical
    draw_line(bitmap, w, h, right_x, baseline, right_x, baseline + height * 0.6f, stroke);
    
    // Bottom curve
    float curve_radius = (right_x - left_x) * 0.5f;
    float curve_center_x = (left_x + right_x) * 0.5f;
    float curve_center_y = baseline + height - curve_radius;
    draw_arc(bitmap, w, h, curve_center_x, curve_center_y, curve_radius, stroke, 3.14f, 0);
}

// Letter V
static void sf_pro_render_v(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    float center_x = w * 0.5f;
    float left_x = w * 0.1f;
    float right_x = w * 0.9f;
    
    // Left diagonal
    draw_line(bitmap, w, h, left_x, baseline, center_x, baseline + height, stroke);
    
    // Right diagonal
    draw_line(bitmap, w, h, right_x, baseline, center_x, baseline + height, stroke);
}

// Letter W
static void sf_pro_render_w(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    float x1 = w * 0.05f;
    float x2 = w * 0.35f;
    float x3 = w * 0.65f;
    float x4 = w * 0.95f;
    
    // First V
    draw_line(bitmap, w, h, x1, baseline, x2, baseline + height, stroke);
    draw_line(bitmap, w, h, x2, baseline + height, x3, baseline + height * 0.4f, stroke);
    
    // Second V
    draw_line(bitmap, w, h, x3, baseline + height * 0.4f, x4, baseline, stroke);
}

// Letter X
static void sf_pro_render_x(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    float left_x = w * 0.15f;
    float right_x = w * 0.85f;
    
    // Forward diagonal
    draw_line(bitmap, w, h, left_x, baseline, right_x, baseline + height, stroke);
    
    // Backward diagonal
    draw_line(bitmap, w, h, right_x, baseline, left_x, baseline + height, stroke);
}

// Letter Y
static void sf_pro_render_y(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    float left_x = w * 0.15f;
    float right_x = w * 0.85f;
    
    if (is_uppercase) {
        // Capital Y
        float junction_y = baseline + cap_height * 0.4f;
        
        // Left diagonal
        draw_line(bitmap, w, h, left_x, baseline, center_x, junction_y, stroke);
        
        // Right diagonal
        draw_line(bitmap, w, h, right_x, baseline, center_x, junction_y, stroke);
        
        // Vertical stem
        draw_line(bitmap, w, h, center_x, junction_y, center_x, baseline + cap_height, stroke);
    } else {
        // Lowercase y - with descender
        float descender_depth = h * SF_PRO_DESCENDER_RATIO;
        
        // Left diagonal
        draw_line(bitmap, w, h, left_x, baseline, center_x, baseline + x_height * 0.6f, stroke);
        
        // Right diagonal (extends to descender)
        draw_line(bitmap, w, h, right_x, baseline, center_x - w * 0.1f, baseline + x_height + descender_depth * 0.8f, stroke);
        
        // Descender curve
        float curve_radius = w * 0.2f;
        draw_arc(bitmap, w, h, center_x - w * 0.1f - curve_radius, baseline + x_height + descender_depth * 0.8f,
                 curve_radius, stroke, 0, 1.57f);
    }
}

// Letter Z
static void sf_pro_render_z(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t x_height, uint32_t baseline, int is_uppercase) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float height = is_uppercase ? cap_height : x_height;
    
    // Top horizontal
    draw_line(bitmap, w, h, w * 0.15f, baseline, w * 0.85f, baseline, stroke);
    
    // Diagonal
    draw_line(bitmap, w, h, w * 0.85f, baseline, w * 0.15f, baseline + height, stroke);
    
    // Bottom horizontal
    draw_line(bitmap, w, h, w * 0.15f, baseline + height, w * 0.85f, baseline + height, stroke);
}

// Digits 0-9
static void sf_pro_render_digit(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline, int digit) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    switch (digit) {
        case 0: {
            // Oval zero
            float center_x = w * 0.5f;
            float center_y = baseline + cap_height * 0.5f;
            float radius_x = w * 0.4f;
            float radius_y = cap_height * 0.48f;
            
            // Draw oval (slightly taller than wide)
            for (float angle = 0; angle < 6.28f; angle += 0.05f) {
                float x = center_x + radius_x * cosf(angle);
                float y = center_y + radius_y * sinf(angle);
                float x_next = center_x + radius_x * cosf(angle + 0.05f);
                float y_next = center_y + radius_y * sinf(angle + 0.05f);
                draw_line(bitmap, w, h, x, y, x_next, y_next, stroke);
            }
            break;
        }
        case 1: {
            // Simple vertical line with small serif at top
            float center_x = w * 0.5f;
            draw_line(bitmap, w, h, center_x, baseline, center_x, baseline + cap_height, stroke);
            
            // Small diagonal at top
            draw_line(bitmap, w, h, center_x - w * 0.15f, baseline + cap_height * 0.15f, 
                     center_x, baseline, stroke);
            break;
        }
        case 2: {
            // Top curve
            float curve_center_x = w * 0.5f;
            float curve_center_y = baseline + cap_height * 0.25f;
            float curve_radius = cap_height * 0.25f;
            draw_arc(bitmap, w, h, curve_center_x, curve_center_y, curve_radius, stroke, 2.5f, 6.8f);
            
            // Diagonal
            draw_line(bitmap, w, h, w * 0.75f, baseline + cap_height * 0.35f,
                     w * 0.25f, baseline + cap_height, stroke);
            
            // Bottom horizontal
            draw_line(bitmap, w, h, w * 0.15f, baseline + cap_height, w * 0.85f, baseline + cap_height, stroke);
            break;
        }
        case 3: {
            // Top curve
            float top_center_x = w * 0.5f;
            float top_center_y = baseline + cap_height * 0.25f;
            float top_radius = cap_height * 0.25f;
            draw_arc(bitmap, w, h, top_center_x, top_center_y, top_radius, stroke, 3.5f, 7.5f);
            
            // Middle junction
            float mid_y = baseline + cap_height * 0.5f;
            draw_line(bitmap, w, h, w * 0.35f, mid_y, w * 0.6f, mid_y, stroke);
            
            // Bottom curve
            float bottom_center_x = w * 0.5f;
            float bottom_center_y = baseline + cap_height * 0.75f;
            float bottom_radius = cap_height * 0.25f;
            draw_arc(bitmap, w, h, bottom_center_x, bottom_center_y, bottom_radius, stroke, -1.0f, 3.0f);
            break;
        }
        case 4: {
            // Vertical line
            float vert_x = w * 0.7f;
            draw_line(bitmap, w, h, vert_x, baseline, vert_x, baseline + cap_height, stroke);
            
            // Diagonal
            draw_line(bitmap, w, h, w * 0.2f, baseline + cap_height * 0.35f,
                     vert_x, baseline + cap_height * 0.65f, stroke);
            
            // Horizontal
            float horiz_y = baseline + cap_height * 0.65f;
            draw_line(bitmap, w, h, w * 0.15f, horiz_y, w * 0.85f, horiz_y, stroke);
            break;
        }
        case 5: {
            // Top horizontal
            draw_line(bitmap, w, h, w * 0.2f, baseline, w * 0.8f, baseline, stroke);
            
            // Left vertical
            draw_line(bitmap, w, h, w * 0.2f, baseline, w * 0.2f, baseline + cap_height * 0.45f, stroke);
            
            // Middle horizontal
            draw_line(bitmap, w, h, w * 0.2f, baseline + cap_height * 0.45f, 
                     w * 0.6f, baseline + cap_height * 0.45f, stroke);
            
            // Bottom curve
            float curve_center_x = w * 0.5f;
            float curve_center_y = baseline + cap_height * 0.7f;
            float curve_radius = cap_height * 0.25f;
            draw_arc(bitmap, w, h, curve_center_x, curve_center_y, curve_radius, stroke, -1.2f, 3.5f);
            break;
        }
        case 6: {
            // Main circular body
            float center_x = w * 0.5f;
            float center_y = baseline + cap_height * 0.65f;
            float radius = cap_height * 0.35f;
            draw_circle(bitmap, w, h, center_x, center_y, radius, stroke);
            
            // Top curve
            draw_arc(bitmap, w, h, center_x - radius * 0.3f, baseline + cap_height * 0.25f,
                    radius * 0.7f, stroke, 2.0f, 4.7f);
            break;
        }
        case 7: {
            // Top horizontal
            draw_line(bitmap, w, h, w * 0.15f, baseline, w * 0.85f, baseline, stroke);
            
            // Diagonal
            draw_line(bitmap, w, h, w * 0.85f, baseline, w * 0.35f, baseline + cap_height, stroke);
            break;
        }
        case 8: {
            // Top circle
            float top_center_x = w * 0.5f;
            float top_center_y = baseline + cap_height * 0.25f;
            float top_radius = cap_height * 0.22f;
            draw_circle(bitmap, w, h, top_center_x, top_center_y, top_radius, stroke);
            
            // Bottom circle (slightly larger)
            float bottom_center_x = w * 0.5f;
            float bottom_center_y = baseline + cap_height * 0.72f;
            float bottom_radius = cap_height * 0.27f;
            draw_circle(bitmap, w, h, bottom_center_x, bottom_center_y, bottom_radius, stroke);
            break;
        }
        case 9: {
            // Main circular body
            float center_x = w * 0.5f;
            float center_y = baseline + cap_height * 0.35f;
            float radius = cap_height * 0.35f;
            draw_circle(bitmap, w, h, center_x, center_y, radius, stroke);
            
            // Bottom curve
            draw_arc(bitmap, w, h, center_x + radius * 0.3f, baseline + cap_height * 0.75f,
                    radius * 0.7f, stroke, 4.7f, 7.4f);
            break;
        }
    }
}

// Punctuation marks
static void sf_pro_render_period(modern_glyph_t* glyph, uint32_t stroke, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    float center_y = baseline + h * 0.9f;
    float radius = stroke * 0.8f;
    
    draw_circle(bitmap, w, h, center_x, center_y, radius, radius * 2);
}

static void sf_pro_render_comma(modern_glyph_t* glyph, uint32_t stroke, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    float dot_y = baseline + h * 0.9f;
    float radius = stroke * 0.8f;
    
    // Dot
    draw_circle(bitmap, w, h, center_x, dot_y, radius, radius * 2);
    
    // Tail
    draw_line(bitmap, w, h, center_x, dot_y + radius, center_x - radius, dot_y + radius * 3, stroke * 0.8f);
}

static void sf_pro_render_exclamation(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    
    // Vertical line
    draw_line(bitmap, w, h, center_x, baseline, center_x, baseline + cap_height * 0.7f, stroke);
    
    // Dot
    float dot_y = baseline + cap_height * 0.9f;
    float radius = stroke * 0.8f;
    draw_circle(bitmap, w, h, center_x, dot_y, radius, radius * 2);
}

static void sf_pro_render_question(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    
    // Top curve
    float curve_center_x = center_x;
    float curve_center_y = baseline + cap_height * 0.2f;
    float curve_radius = cap_height * 0.2f;
    draw_arc(bitmap, w, h, curve_center_x, curve_center_y, curve_radius, stroke, 2.5f, 7.0f);
    
    // Middle curve
    draw_arc(bitmap, w, h, curve_center_x, curve_center_y + curve_radius * 1.5f, 
             curve_radius * 0.8f, stroke, -0.5f, 2.0f);
    
    // Dot
    float dot_y = baseline + cap_height * 0.9f;
    float radius = stroke * 0.8f;
    draw_circle(bitmap, w, h, center_x, dot_y, radius, radius * 2);
}

static void sf_pro_render_colon(modern_glyph_t* glyph, uint32_t stroke, uint32_t x_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    float radius = stroke * 0.8f;
    
    // Top dot
    float top_y = baseline + x_height * 0.3f;
    draw_circle(bitmap, w, h, center_x, top_y, radius, radius * 2);
    
    // Bottom dot
    float bottom_y = baseline + x_height * 0.9f;
    draw_circle(bitmap, w, h, center_x, bottom_y, radius, radius * 2);
}

static void sf_pro_render_semicolon(modern_glyph_t* glyph, uint32_t stroke, uint32_t x_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    float radius = stroke * 0.8f;
    
    // Top dot
    float top_y = baseline + x_height * 0.3f;
    draw_circle(bitmap, w, h, center_x, top_y, radius, radius * 2);
    
    // Bottom comma
    float comma_y = baseline + x_height * 0.9f;
    draw_circle(bitmap, w, h, center_x, comma_y, radius, radius * 2);
    draw_line(bitmap, w, h, center_x, comma_y + radius, center_x - radius, comma_y + radius * 3, stroke * 0.8f);
}

// Additional punctuation
static void sf_pro_render_apostrophe(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    float y = baseline + cap_height * 0.1f;
    
    draw_line(bitmap, w, h, center_x, y, center_x - stroke * 0.5f, y + stroke * 2, stroke);
}

static void sf_pro_render_quote(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float left_x = w * 0.3f;
    float right_x = w * 0.7f;
    float y = baseline + cap_height * 0.1f;
    
    // Left quote mark
    draw_line(bitmap, w, h, left_x, y, left_x - stroke * 0.5f, y + stroke * 2, stroke);
    
    // Right quote mark
    draw_line(bitmap, w, h, right_x, y, right_x - stroke * 0.5f, y + stroke * 2, stroke);
}

static void sf_pro_render_hyphen(modern_glyph_t* glyph, uint32_t stroke, uint32_t x_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float y = baseline + x_height * 0.5f;
    draw_line(bitmap, w, h, w * 0.2f, y, w * 0.8f, y, stroke);
}

static void sf_pro_render_plus(modern_glyph_t* glyph, uint32_t stroke, uint32_t x_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float center_x = w * 0.5f;
    float center_y = baseline + x_height * 0.5f;
    float half_size = x_height * 0.35f;
    
    // Horizontal
    draw_line(bitmap, w, h, center_x - half_size, center_y, center_x + half_size, center_y, stroke);
    
    // Vertical
    draw_line(bitmap, w, h, center_x, center_y - half_size, center_x, center_y + half_size, stroke);
}

static void sf_pro_render_equals(modern_glyph_t* glyph, uint32_t stroke, uint32_t x_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    float y1 = baseline + x_height * 0.35f;
    float y2 = baseline + x_height * 0.65f;
    
    draw_line(bitmap, w, h, w * 0.2f, y1, w * 0.8f, y1, stroke);
    draw_line(bitmap, w, h, w * 0.2f, y2, w * 0.8f, y2, stroke);
}

static void sf_pro_render_slash(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    draw_line(bitmap, w, h, w * 0.2f, baseline + cap_height, w * 0.8f, baseline, stroke);
}

static void sf_pro_render_backslash(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    draw_line(bitmap, w, h, w * 0.2f, baseline, w * 0.8f, baseline + cap_height, stroke);
}

static void sf_pro_render_missing(modern_glyph_t* glyph, uint32_t stroke, uint32_t cap_height, uint32_t baseline) {
    uint8_t* bitmap = glyph->bitmap_data;
    uint32_t w = glyph->width;
    uint32_t h = glyph->height;
    
    // Draw a box with an X to indicate missing glyph
    float margin = w * 0.1f;
    
    // Box
    draw_line(bitmap, w, h, margin, baseline, w - margin, baseline, stroke);
    draw_line(bitmap, w, h, w - margin, baseline, w - margin, baseline + cap_height, stroke);
    draw_line(bitmap, w, h, w - margin, baseline + cap_height, margin, baseline + cap_height, stroke);
    draw_line(bitmap, w, h, margin, baseline + cap_height, margin, baseline, stroke);
    
    // X
    draw_line(bitmap, w, h, margin, baseline, w - margin, baseline + cap_height, stroke * 0.8f);
    draw_line(bitmap, w, h, w - margin, baseline, margin, baseline + cap_height, stroke * 0.8f);
}

// === Main glyph generation function ===
static void generate_sf_pro_glyph(modern_glyph_t* glyph, char c, uint32_t size, font_weight_t weight) {
    uint32_t stroke = sf_pro_stroke_width(size, weight);
    
    // SF Pro proportions
    uint32_t cap_height = size * SF_PRO_CAP_HEIGHT_RATIO;
    uint32_t x_height = size * SF_PRO_X_HEIGHT_RATIO;
    uint32_t baseline = size * 0.2f;
    
    // Default dimensions
    uint32_t width = size * 0.6f;
    uint32_t height = size;
    
    // Adjust width based on character
    switch (c) {
        case 'i': case 'j': case 'l': case '1': case '!': case '\'': case '.': case ',': case ':': case ';':
            width = size * 0.25f; break;
        case 'f': case 't': case 'r':
            width = size * 0.4f; break;
        case 'm': case 'w': case 'M': case 'W':
            width = size * 0.9f; break;
        case ' ':
            width = size * 0.3f; break;
        case '"': case '-': case '+': case '=':
            width = size * 0.5f; break;
        default:
            if (c >= 'A' && c <= 'Z') width = size * 0.7f;
            break;
    }
    
    glyph->width = width;
    glyph->height = height;
    glyph->advance_width = width + (uint32_t)(size * SF_PRO_LETTER_SPACING);
    glyph->left_bearing = 0;
    glyph->top_bearing = 0;
    glyph->antialiased = 1;
    
    size_t bitmap_size = width * height;
    glyph->bitmap_data = (uint8_t*)kmalloc(bitmap_size);
    if (!glyph->bitmap_data) return;
    
    // Clear bitmap
    for (uint32_t i = 0; i < bitmap_size; i++) {
        glyph->bitmap_data[i] = 0;
    }
    
    // Render character
    switch (c) {
        // Letters
        case 'a': case 'A': sf_pro_render_a(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'b': case 'B': sf_pro_render_b(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'c': case 'C': sf_pro_render_c(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'd': case 'D': sf_pro_render_d(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'e': case 'E': sf_pro_render_e(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'f': case 'F': sf_pro_render_f(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'g': case 'G': sf_pro_render_g(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'h': case 'H': sf_pro_render_h(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'i': case 'I': sf_pro_render_i(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'j': case 'J': sf_pro_render_j(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'k': case 'K': sf_pro_render_k(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'l': case 'L': sf_pro_render_l(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'm': case 'M': sf_pro_render_m(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'n': case 'N': sf_pro_render_n(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'o': case 'O': sf_pro_render_o(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'p': case 'P': sf_pro_render_p(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'q': case 'Q': sf_pro_render_q(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'r': case 'R': sf_pro_render_r(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 's': case 'S': sf_pro_render_s(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 't': case 'T': sf_pro_render_t(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'u': case 'U': sf_pro_render_u(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'v': case 'V': sf_pro_render_v(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'w': case 'W': sf_pro_render_w(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'x': case 'X': sf_pro_render_x(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'y': case 'Y': sf_pro_render_y(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        case 'z': case 'Z': sf_pro_render_z(glyph, stroke, cap_height, x_height, baseline, c >= 'A'); break;
        
        // Digits
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
            sf_pro_render_digit(glyph, stroke, cap_height, baseline, c - '0');
            break;
            
        // Punctuation
        case '.': sf_pro_render_period(glyph, stroke, baseline); break;
        case ',': sf_pro_render_comma(glyph, stroke, baseline); break;
        case '!': sf_pro_render_exclamation(glyph, stroke, cap_height, baseline); break;
        case '?': sf_pro_render_question(glyph, stroke, cap_height, baseline); break;
        case ':': sf_pro_render_colon(glyph, stroke, x_height, baseline); break;
        case ';': sf_pro_render_semicolon(glyph, stroke, x_height, baseline); break;
        case '\'': sf_pro_render_apostrophe(glyph, stroke, cap_height, baseline); break;
        case '"': sf_pro_render_quote(glyph, stroke, cap_height, baseline); break;
        case '-': sf_pro_render_hyphen(glyph, stroke, x_height, baseline); break;
        case '+': sf_pro_render_plus(glyph, stroke, x_height, baseline); break;
        case '=': sf_pro_render_equals(glyph, stroke, x_height, baseline); break;
        case '/': sf_pro_render_slash(glyph, stroke, cap_height, baseline); break;
        case '\\': sf_pro_render_backslash(glyph, stroke, cap_height, baseline); break;
        
        case ' ':
            // Space - no rendering needed
            break;
            
        default:
            // Unknown character - render missing glyph
            sf_pro_render_missing(glyph, stroke, cap_height, baseline);
            break;
    }
}

// === Font Creation ===
modern_font_t* modern_font_create_sf_pro(uint32_t size, font_weight_t weight) {
    modern_font_t* font = (modern_font_t*)kmalloc(sizeof(modern_font_t));
    if (!font) return NULL;
    
    // Set font properties
    strcpy(font->font_name, "SF Pro Complete");
    font->font_size = size;
    font->weight = weight;
    font->line_height = size * 1.2f;
    font->baseline = size * 0.8f;
    font->cap_height = size * SF_PRO_CAP_HEIGHT_RATIO;
    font->x_height = size * SF_PRO_X_HEIGHT_RATIO;
    
    // Initialize all glyphs as empty
    for (int i = 0; i < 256; i++) {
        font->glyphs[i].bitmap_data = NULL;
        font->glyphs[i].width = 0;
        font->glyphs[i].height = 0;
    }
    
    // Generate printable ASCII characters (32-126)
    for (int i = 32; i <= 126; i++) {
        generate_sf_pro_glyph(&font->glyphs[i], (char)i, size, weight);
    }
    
    return font;
}

// === System Font Functions ===
modern_font_t* modern_font_create_system_default(void) {
    return modern_font_create_sf_pro(16, FONT_WEIGHT_REGULAR);
}

modern_font_t* modern_font_get_system_font(void) {
    return system_font;
}

void modern_font_set_system_font(modern_font_t* font) {
    system_font = font;
}

// === Rendering Functions ===
void modern_font_render_glyph_antialiased(metal_device_t* device, modern_glyph_t* glyph,
                                         uint32_t x, uint32_t y, color32_t color) {
    if (!device || !glyph || !glyph->bitmap_data) return;
    
    for (uint32_t gy = 0; gy < glyph->height; gy++) {
        for (uint32_t gx = 0; gx < glyph->width; gx++) {
            uint8_t alpha = glyph->bitmap_data[gy * glyph->width + gx];
            if (alpha > 0) {
                uint32_t px = x + gx;
                uint32_t py = y + gy;
                
                if (alpha == 255) {
                    metal_set_pixel(device, px, py, color);
                } else {
                    // Antialiasing - blend with background
                    color32_t bg = metal_get_pixel(device, px, py);
                    color32_t blended = blend_color(bg, color, alpha);
                    metal_set_pixel(device, px, py, blended);
                }
            }
        }
    }
}

void modern_font_render_text(metal_device_t* device, modern_font_t* font, 
                            const char* text, uint32_t x, uint32_t y, color32_t color) {
    if (!device || !font || !text) return;
    
    uint32_t current_x = x;
    uint32_t current_y = y;
    size_t len = strlen(text);
    
    for (size_t i = 0; i < len; i++) {
        char c = text[i];
        
        if (c == '\n') {
            current_x = x;
            current_y += font->line_height;
            continue;
        }
        
        if (c >= 0 && c < 256) {
            modern_glyph_t* glyph = &font->glyphs[(int)c];
            if (glyph->bitmap_data) {
                modern_font_render_glyph_antialiased(device, glyph, current_x, current_y, color);
            }
            current_x += glyph->advance_width;
        }
    }
}

// === Initialization and Cleanup ===
int modern_font_init(void) {
    if (font_system_initialized) return 1;
    
    serial_writestring("SF_PRO_COMPLETE: Initializing complete SF Pro font system\n");
    
    system_font = modern_font_create_system_default();
    if (!system_font) {
        serial_writestring("SF_PRO_COMPLETE: Failed to create system font\n");
        return 0;
    }
    
    font_system_initialized = 1;
    serial_writestring("SF_PRO_COMPLETE: Font system initialized successfully\n");
    return 1;
}

void modern_font_destroy(modern_font_t* font) {
    if (!font) return;
    
    for (int i = 0; i < 256; i++) {
        if (font->glyphs[i].bitmap_data) {
            kfree(font->glyphs[i].bitmap_data);
        }
    }
    
    kfree(font);
}

void modern_font_cleanup(void) {
    if (system_font) {
        modern_font_destroy(system_font);
        system_font = NULL;
    }
    font_system_initialized = 0;
}