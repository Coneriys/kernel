#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include <stddef.h>

// === Modern Video System for MyKernel OS ===

// Text mode (fallback only)
#define VGA_TEXT_MEMORY    0xB8000
#define VGA_TEXT_WIDTH     80
#define VGA_TEXT_HEIGHT    25

// === Primary High-Resolution Modes ===

// Bochs VBE (QEMU/Virtual environments) - Primary target
#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_LFB_ENABLED           0x40

// QEMU Bochs VGA PCI IDs for framebuffer detection
#define QEMU_VENDOR_ID                  0x1234
#define QEMU_VGA_DEVICE_ID              0x1111

// Standard HD resolutions (priority order)
typedef enum {
    VIDEO_MODE_TEXT,                    // Text mode fallback
    VIDEO_MODE_HD_1080P,               // 1920x1080x32 - Full HD (primary)
    VIDEO_MODE_HD_720P,                // 1280x720x32  - HD Ready
    VIDEO_MODE_HD_1440P,               // 2560x1440x32 - 2K
    VIDEO_MODE_VESA_1024x768,          // 1024x768x32  - XGA
    VIDEO_MODE_VESA_800x600,           // 800x600x32   - SVGA
    VIDEO_MODE_VGA_FALLBACK            // 320x200x8    - Emergency fallback
} video_mode_t;

// Modern color depth (32-bit ARGB primary)
typedef struct {
    uint8_t b;  // Blue
    uint8_t g;  // Green  
    uint8_t r;  // Red
    uint8_t a;  // Alpha (transparency)
} color32_t;

// Legacy 24-bit RGB for compatibility
typedef struct {
    uint8_t r;
    uint8_t g; 
    uint8_t b;
} color24_t;

// Video driver structure for modern graphics
typedef struct {
    video_mode_t current_mode;
    uint32_t width;
    uint32_t height;
    uint8_t depth;                     // Bits per pixel (32, 24, 16, 8)
    uint32_t framebuffer;              // Physical framebuffer address
    uint32_t pitch;                    // Bytes per line
    uint32_t framebuffer_size;         // Total framebuffer size in bytes
    
    // Advanced features
    uint8_t hardware_acceleration;     // Hardware 2D/3D support
    uint8_t double_buffering;          // Double buffer support
    uint32_t back_buffer;              // Back buffer address
    
    // VBE specific
    uint16_t vbe_version;
    uint32_t linear_framebuffer;       // Linear framebuffer address
    
    // Performance optimizations
    uint8_t memory_mapped;             // Direct memory access
    uint8_t vsync_enabled;             // Vertical sync
    
} video_driver_t;

// Modern video mode capabilities
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t depth;
    uint32_t frequency;                // Refresh rate in Hz
    const char* name;
    uint8_t available;
} video_mode_info_t;

// === Core Video Functions ===

// System initialization
void video_init(void);
void video_shutdown(void);

// Mode management (prioritizing HD modes)
int video_set_mode(video_mode_t mode);
video_mode_t video_detect_best_mode(void);
video_mode_info_t* video_get_mode_info(video_mode_t mode);
video_driver_t* video_get_driver(void);

// === High-Resolution Drawing Functions ===

// Modern 32-bit drawing
void video_put_pixel32(uint32_t x, uint32_t y, color32_t color);
color32_t video_get_pixel32(uint32_t x, uint32_t y);
void video_fill_rect32(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color32_t color);
void video_draw_line32(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, color32_t color);

// Anti-aliased drawing
void video_draw_line_aa(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, color32_t color);
void video_draw_circle_aa(uint32_t cx, uint32_t cy, uint32_t radius, color32_t color);
void video_fill_circle_aa(uint32_t cx, uint32_t cy, uint32_t radius, color32_t color);

// Advanced graphics
void video_draw_rounded_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                            uint32_t radius, color32_t color);
void video_fill_rounded_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                            uint32_t radius, color32_t color);

// Gradient and effects
void video_draw_gradient_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                             color32_t color1, color32_t color2, uint8_t direction);
void video_draw_shadow(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                      uint32_t blur_radius, color32_t shadow_color);

// === Bochs VBE Driver (Primary) ===
int bochs_vbe_init(void);
int bochs_vbe_set_mode(uint32_t width, uint32_t height, uint8_t depth);
int bochs_vbe_detect(void);
uint32_t bochs_vbe_get_framebuffer(void);

// === VESA BIOS Extensions ===
int vesa_init(void);
int vesa_set_mode(uint16_t mode);
int vesa_detect_modes(void);

// === Legacy VGA (Emergency fallback only) ===
void vga_init_fallback(void);
int vga_set_mode_13h(void);
void vga_put_pixel_fallback(uint16_t x, uint16_t y, uint8_t color);

// === Color Utilities ===
color32_t make_color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color32_t color_blend(color32_t fg, color32_t bg, uint8_t alpha);
color32_t color_interpolate(color32_t c1, color32_t c2, float t);

// macOS-style color palette
#define COLOR_MACOS_BLUE        make_color32(0, 122, 255, 255)
#define COLOR_MACOS_GRAY        make_color32(142, 142, 147, 255)
#define COLOR_MACOS_GREEN       make_color32(52, 199, 89, 255)
#define COLOR_MACOS_YELLOW      make_color32(255, 204, 0, 255)
#define COLOR_MACOS_ORANGE      make_color32(255, 149, 0, 255)
#define COLOR_MACOS_RED         make_color32(255, 59, 48, 255)
#define COLOR_MACOS_PURPLE      make_color32(175, 82, 222, 255)
#define COLOR_MACOS_PINK        make_color32(255, 45, 85, 255)

#define COLOR_MACOS_WINDOW_BG   make_color32(246, 246, 246, 255)
#define COLOR_MACOS_SIDEBAR_BG  make_color32(237, 237, 237, 255)
#define COLOR_MACOS_SELECTION   make_color32(0, 99, 225, 255)
#define COLOR_MACOS_TEXT        make_color32(28, 28, 30, 255)
#define COLOR_MACOS_SECONDARY   make_color32(99, 99, 102, 255)

// === Performance and Debug ===
void video_benchmark(void);
void video_test_patterns(void);
void video_show_info(void);

#endif