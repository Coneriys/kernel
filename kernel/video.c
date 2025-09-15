// === MyKernel OS - Modern High-Resolution Video System ===
// Priority: HD resolutions with macOS-style rendering
// Fallback: VGA only if nothing else works

#include "../include/video.h"
#include "../include/pci.h"

extern void terminal_writestring(const char* data);
extern void serial_writestring(const char* str);

// === Modern Video Driver ===
static video_driver_t video_driver = {0};
static int bochs_vbe_available = 0;
static int vesa_available = 0;

// HD Mode definitions (priority order)
static video_mode_info_t video_modes[] = {
    {1920, 1080, 32, 60, "Full HD 1080p", 0},     // Primary target
    {1280, 720,  32, 60, "HD Ready 720p", 0},     // Secondary
    {2560, 1440, 32, 60, "2K QHD", 0},           // High-end
    {1024, 768,  32, 60, "XGA", 0},              // Compatibility
    {800,  600,  32, 60, "SVGA", 0},             // Basic
    {320,  200,  8,  60, "VGA Fallback", 1}      // Emergency only
};

// === Core System Functions ===

void video_init(void) {
    terminal_writestring("MyKernel Video System v2.0 - HD Graphics\n");
    serial_writestring("VIDEO: Starting modern video system initialization\n");
    
    // Initialize PCI for hardware detection
    pci_init();
    
    // Try AMD GPU first for modern hardware support (RX 6600 XT, etc.)
    extern int detect_amd_gpu(void);
    extern void amd_gpu_init(void);
    if (detect_amd_gpu()) {
        serial_writestring("VIDEO: Modern AMD GPU detected, initializing...\n");
        amd_gpu_init();
    }
    
    // Try Bochs VBE first (best for QEMU/virtual environments)
    if (bochs_vbe_init()) {
        terminal_writestring("Bochs VBE detected - HD modes available\n");
        bochs_vbe_available = 1;
    }
    
    // Try VESA as secondary option
    if (vesa_init()) {
        terminal_writestring("VESA BIOS Extensions detected\n");
        vesa_available = 1;
    }
    
    // Set default text mode
    video_driver.current_mode = VIDEO_MODE_TEXT;
    video_driver.width = VGA_TEXT_WIDTH;
    video_driver.height = VGA_TEXT_HEIGHT;
    video_driver.depth = 4;
    video_driver.framebuffer = VGA_TEXT_MEMORY;
    video_driver.pitch = VGA_TEXT_WIDTH * 2;
    
    // Detect and set best available mode
    video_mode_t best_mode = video_detect_best_mode();
    
    if (best_mode != VIDEO_MODE_TEXT) {
        terminal_writestring("HD graphics available - use 'gui' command to activate\n");
    } else {
        terminal_writestring("Only text mode available\n");
    }
    
    serial_writestring("VIDEO: Initialization complete\n");
}

video_mode_t video_detect_best_mode(void) {
    serial_writestring("VIDEO: Detecting best available mode\n");
    
    // Just check if we have VBE support - don't actually switch modes yet
    if (bochs_vbe_available) {
        video_modes[0].available = 1; // 1080p
        video_modes[1].available = 1; // 720p  
        video_modes[3].available = 1; // 1024x768
        serial_writestring("VIDEO: Bochs VBE available - HD modes supported\n");
        return VIDEO_MODE_HD_1080P; // Best available mode
    }
    
    if (vesa_available) {
        video_modes[3].available = 1; // 1024x768
        serial_writestring("VIDEO: VESA available - XGA mode supported\n");
        return VIDEO_MODE_VESA_1024x768;
    }
    
    // Fallback to text mode
    serial_writestring("VIDEO: Only text mode available\n");
    return VIDEO_MODE_TEXT;
}

int video_set_mode(video_mode_t mode) {
    serial_writestring("VIDEO: Setting video mode\n");
    
    switch (mode) {
        case VIDEO_MODE_TEXT:
            video_driver.current_mode = VIDEO_MODE_TEXT;
            video_driver.width = VGA_TEXT_WIDTH;
            video_driver.height = VGA_TEXT_HEIGHT;
            video_driver.depth = 4;
            video_driver.framebuffer = VGA_TEXT_MEMORY;
            video_driver.pitch = VGA_TEXT_WIDTH * 2;
            return 1;
            
        case VIDEO_MODE_HD_1080P:
            if (bochs_vbe_available && bochs_vbe_set_mode(1920, 1080, 32)) {
                video_driver.current_mode = VIDEO_MODE_HD_1080P;
                video_driver.width = 1920;
                video_driver.height = 1080;
                video_driver.depth = 32;
                video_driver.framebuffer = bochs_vbe_get_framebuffer();
                video_driver.pitch = 1920 * 4;
                video_driver.framebuffer_size = 1920 * 1080 * 4;
                serial_writestring("VIDEO: 1080p Full HD mode set\n");
                return 1;
            }
            break;
            
        case VIDEO_MODE_HD_720P:
            if (bochs_vbe_available && bochs_vbe_set_mode(1280, 720, 32)) {
                video_driver.current_mode = VIDEO_MODE_HD_720P;
                video_driver.width = 1280;
                video_driver.height = 720;
                video_driver.depth = 32;
                video_driver.framebuffer = bochs_vbe_get_framebuffer();
                video_driver.pitch = 1280 * 4;
                video_driver.framebuffer_size = 1280 * 720 * 4;
                serial_writestring("VIDEO: 720p HD mode set\n");
                return 1;
            }
            break;
            
        case VIDEO_MODE_VESA_1024x768:
            if (bochs_vbe_available && bochs_vbe_set_mode(1024, 768, 32)) {
                video_driver.current_mode = VIDEO_MODE_VESA_1024x768;
                video_driver.width = 1024;
                video_driver.height = 768;
                video_driver.depth = 32;
                video_driver.framebuffer = bochs_vbe_get_framebuffer();
                video_driver.pitch = 1024 * 4;
                video_driver.framebuffer_size = 1024 * 768 * 4;
                serial_writestring("VIDEO: 1024x768 XGA mode set\n");
                return 1;
            }
            break;
            
        case VIDEO_MODE_VGA_FALLBACK:
            // Emergency fallback only
            serial_writestring("VIDEO: Using VGA fallback mode\n");
            if (vga_set_mode_13h()) {
                video_driver.current_mode = VIDEO_MODE_VGA_FALLBACK;
                video_driver.width = 320;
                video_driver.height = 200;
                video_driver.depth = 8;
                video_driver.framebuffer = 0xA0000;
                video_driver.pitch = 320;
                video_driver.framebuffer_size = 320 * 200;
                return 1;
            }
            break;
            
        default:
            break;
    }
    
    serial_writestring("VIDEO: Failed to set requested mode\n");
    return 0;
}

// === Modern 32-bit Drawing Functions ===

inline void video_put_pixel32(uint32_t x, uint32_t y, color32_t color) {
    if (x >= video_driver.width || y >= video_driver.height) return;
    
    if (video_driver.depth == 32) {
        uint32_t* fb = (uint32_t*)video_driver.framebuffer;
        uint32_t offset = y * (video_driver.pitch / 4) + x;
        // Pack color as RGBA for framebuffer  
        uint32_t packed = (color.a << 24) | (color.b << 16) | (color.g << 8) | color.r;
        fb[offset] = packed;
    } else if (video_driver.depth == 8) {
        // Fallback for VGA mode
        uint8_t* fb = (uint8_t*)video_driver.framebuffer;
        uint8_t vga_color = (color.r >> 5) << 5 | (color.g >> 5) << 2 | (color.b >> 6);
        fb[y * video_driver.pitch + x] = vga_color;
    }
}

color32_t video_get_pixel32(uint32_t x, uint32_t y) {
    color32_t color = {0, 0, 0, 255};
    
    if (x >= video_driver.width || y >= video_driver.height) return color;
    
    if (video_driver.depth == 32) {
        uint32_t* fb = (uint32_t*)video_driver.framebuffer;
        uint32_t offset = y * (video_driver.pitch / 4) + x;
        color = *(color32_t*)&fb[offset];
    }
    
    return color;
}

void video_fill_rect32(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color32_t color) {
    for (uint32_t dy = 0; dy < height; dy++) {
        for (uint32_t dx = 0; dx < width; dx++) {
            video_put_pixel32(x + dx, y + dy, color);
        }
    }
}

void video_draw_line32(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, color32_t color) {
    // Bresenham's line algorithm
    int dx = (x2 > x1) ? x2 - x1 : x1 - x2;
    int dy = (y2 > y1) ? y2 - y1 : y1 - y2;
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    uint32_t x = x1, y = y1;
    
    while (1) {
        video_put_pixel32(x, y, color);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

// === Advanced Graphics (macOS-style) ===

void video_draw_rounded_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                            uint32_t radius, color32_t color) {
    // Draw rounded rectangle outline
    
    // Top and bottom horizontal lines
    for (uint32_t i = radius; i < width - radius; i++) {
        video_put_pixel32(x + i, y, color);                    // Top
        video_put_pixel32(x + i, y + height - 1, color);      // Bottom
    }
    
    // Left and right vertical lines
    for (uint32_t i = radius; i < height - radius; i++) {
        video_put_pixel32(x, y + i, color);                   // Left
        video_put_pixel32(x + width - 1, y + i, color);       // Right
    }
    
    // Draw corner arcs (simplified)
    for (uint32_t i = 0; i < radius; i++) {
        for (uint32_t j = 0; j < radius; j++) {
            uint32_t dx = radius - i;
            uint32_t dy = radius - j;
            if (dx * dx + dy * dy <= radius * radius && 
                dx * dx + dy * dy >= (radius - 1) * (radius - 1)) {
                
                // Top-left
                video_put_pixel32(x + i, y + j, color);
                // Top-right
                video_put_pixel32(x + width - 1 - i, y + j, color);
                // Bottom-left
                video_put_pixel32(x + i, y + height - 1 - j, color);
                // Bottom-right
                video_put_pixel32(x + width - 1 - i, y + height - 1 - j, color);
            }
        }
    }
}

void video_fill_rounded_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                            uint32_t radius, color32_t color) {
    // Handle edge cases
    if (radius == 0) {
        video_fill_rect32(x, y, width, height, color);
        return;
    }
    
    if (radius * 2 > width) radius = width / 2;
    if (radius * 2 > height) radius = height / 2;
    
    // Fill main rectangle (avoiding corners)
    video_fill_rect32(x + radius, y, width - 2 * radius, height, color);
    video_fill_rect32(x, y + radius, radius, height - 2 * radius, color);
    video_fill_rect32(x + width - radius, y + radius, radius, height - 2 * radius, color);
    
    // Draw anti-aliased corners
    int r_sq = radius * radius;
    int r_sq_inner = (radius - 1) * (radius - 1);
    
    for (int i = 0; i <= (int)radius + 1; i++) {
        for (int j = 0; j <= (int)radius + 1; j++) {
            int dx = radius - i;
            int dy = radius - j;
            int dist_sq = dx * dx + dy * dy;
            
            if (dist_sq <= r_sq) {
                color32_t final_color = color;
                
                // Anti-aliasing for corner edges
                if (dist_sq > r_sq_inner) {
                    // Simple integer-based anti-aliasing
                    int edge_distance = r_sq - dist_sq;
                    if (edge_distance < r_sq / 4) { // Close to edge
                        uint8_t alpha = (uint8_t)((edge_distance * 255) / (r_sq / 4));
                        final_color.a = (uint8_t)((color.a * alpha) / 255);
                    }
                }
                
                // Apply to all four corners
                uint32_t corners[4][2] = {
                    {x + i, y + j},                           // Top-left
                    {x + width - 1 - i, y + j},               // Top-right  
                    {x + i, y + height - 1 - j},              // Bottom-left
                    {x + width - 1 - i, y + height - 1 - j}   // Bottom-right
                };
                
                video_driver_t* driver = video_get_driver();
                for (int c = 0; c < 4; c++) {
                    uint32_t px = corners[c][0];
                    uint32_t py = corners[c][1];
                    
                    if (px < driver->width && py < driver->height) {
                        if (final_color.a == 255) {
                            video_put_pixel32(px, py, final_color);
                        } else {
                            color32_t bg = video_get_pixel32(px, py);
                            color32_t blended = color_blend(final_color, bg, final_color.a);
                            video_put_pixel32(px, py, blended);
                        }
                    }
                }
            }
        }
    }
}

void video_draw_gradient_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                             color32_t color1, color32_t color2, uint8_t direction) {
    // Fast gradient using integer math and line drawing
    if (direction == 0) { // Horizontal gradient
        uint32_t steps = width / 4; // Reduce detail for performance
        if (steps < 8) steps = 8;
        for (uint32_t i = 0; i < steps; i++) {
            uint32_t band_x = x + (i * width) / steps;
            uint32_t band_width = width / steps + 1;
            uint32_t t256 = (i * 256) / steps; // Fixed point math
            
            color32_t color;
            color.r = (color1.r * (256 - t256) + color2.r * t256) >> 8;
            color.g = (color1.g * (256 - t256) + color2.g * t256) >> 8;
            color.b = (color1.b * (256 - t256) + color2.b * t256) >> 8;
            color.a = (color1.a * (256 - t256) + color2.a * t256) >> 8;
            
            video_fill_rect32(band_x, y, band_width, height, color);
        }
    } else { // Vertical gradient
        uint32_t steps = height / 4; // Reduce detail for performance
        if (steps < 8) steps = 8;
        for (uint32_t i = 0; i < steps; i++) {
            uint32_t band_y = y + (i * height) / steps;
            uint32_t band_height = height / steps + 1;
            uint32_t t256 = (i * 256) / steps; // Fixed point math
            
            color32_t color;
            color.r = (color1.r * (256 - t256) + color2.r * t256) >> 8;
            color.g = (color1.g * (256 - t256) + color2.g * t256) >> 8;
            color.b = (color1.b * (256 - t256) + color2.b * t256) >> 8;
            color.a = (color1.a * (256 - t256) + color2.a * t256) >> 8;
            
            video_fill_rect32(x, band_y, width, band_height, color);
        }
    }
}

// === Bochs VBE Driver (Primary for QEMU) ===

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int bochs_vbe_detect(void) {
    // Test if Bochs VBE is available
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ID);
    uint16_t id = inw(VBE_DISPI_IOPORT_DATA);
    
    return (id == 0xB0C0 || id == 0xB0C1 || id == 0xB0C2 || id == 0xB0C3 || id == 0xB0C4 || id == 0xB0C5);
}

int bochs_vbe_init(void) {
    if (!bochs_vbe_detect()) {
        serial_writestring("VIDEO: Bochs VBE not detected\n");
        return 0;
    }
    
    serial_writestring("VIDEO: Bochs VBE initialized successfully\n");
    return 1;
}

int bochs_vbe_set_mode(uint32_t width, uint32_t height, uint8_t depth) {
    if (!bochs_vbe_available) return 0;
    
    serial_writestring("VIDEO: Setting Bochs VBE mode\n");
    
    // Disable VBE
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
    outw(VBE_DISPI_IOPORT_DATA, VBE_DISPI_DISABLED);
    
    // Set resolution
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES);
    outw(VBE_DISPI_IOPORT_DATA, width);
    
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES);
    outw(VBE_DISPI_IOPORT_DATA, height);
    
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP);
    outw(VBE_DISPI_IOPORT_DATA, depth);
    
    // Set bank to 0
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BANK);
    outw(VBE_DISPI_IOPORT_DATA, 0);
    
    // Enable VBE with Linear Frame Buffer
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
    outw(VBE_DISPI_IOPORT_DATA, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED | 0x40);
    
    return 1;
}

uint32_t bochs_vbe_get_framebuffer(void) {
    extern int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev);
    
    // First check if LFB is enabled
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
    uint16_t enabled = inw(VBE_DISPI_IOPORT_DATA);
    
    if (!(enabled & VBE_DISPI_ENABLED)) {
        serial_writestring("VIDEO: VBE not enabled!\n");
        return 0xA0000;
    }
    
    // Check if we can access extended memory through LFB
    if (enabled & VBE_DISPI_LFB_ENABLED) {
        // LFB is enabled - use PCI to find the actual framebuffer address
        pci_device_t qemu_vga;
        
        if (pci_find_device(QEMU_VENDOR_ID, QEMU_VGA_DEVICE_ID, &qemu_vga) == 0 && 
            qemu_vga.base_addresses[0] != 0) {
            // Use BAR0 as framebuffer (mask off status bits)
            uint32_t fb_addr = qemu_vga.base_addresses[0] & 0xFFFFFFF0;
            serial_writestring("VIDEO: Found QEMU VGA device - using PCI BAR0 framebuffer\n");
            return fb_addr;
        } else {
            // Fallback to standard QEMU addresses if PCI detection fails
            serial_writestring("VIDEO: PCI detection failed - trying standard LFB addresses\n");
            
            // Try the most common QEMU framebuffer addresses
            uint32_t standard_addrs[] = {0xFD000000, 0xFC000000, 0xE0000000, 0};
            
            for (int i = 0; standard_addrs[i] != 0; i++) {
                // Simple memory test - try to write/read a test pattern
                volatile uint32_t* test_addr = (volatile uint32_t*)standard_addrs[i];
                uint32_t original = *test_addr;
                *test_addr = 0xDEADBEEF;
                
                if (*test_addr == 0xDEADBEEF) {
                    *test_addr = original;  // Restore original value
                    serial_writestring("VIDEO: Using LFB at detected address\n");
                    return standard_addrs[i];
                }
                *test_addr = original;  // Restore even if test failed
            }
            
            serial_writestring("VIDEO: LFB detection failed - using fallback\n");
            return 0xFC000000;  // Last resort fallback
        }
    } else {
        serial_writestring("VIDEO: Using VGA memory at 0xA0000 with banking\n");
        return 0xA0000;
    }
}

// === Color Utilities ===

color32_t make_color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    color32_t color = {b, g, r, a};
    return color;
}

color32_t color_blend(color32_t fg, color32_t bg, uint8_t alpha) {
    color32_t result;
    uint16_t inv_alpha = 255 - alpha;
    
    result.r = (fg.r * alpha + bg.r * inv_alpha) / 255;
    result.g = (fg.g * alpha + bg.g * inv_alpha) / 255;
    result.b = (fg.b * alpha + bg.b * inv_alpha) / 255;
    result.a = 255;
    
    return result;
}

color32_t color_interpolate(color32_t c1, color32_t c2, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    color32_t result;
    result.r = (uint8_t)(c1.r + (c2.r - c1.r) * t);
    result.g = (uint8_t)(c1.g + (c2.g - c1.g) * t);
    result.b = (uint8_t)(c1.b + (c2.b - c1.b) * t);
    result.a = (uint8_t)(c1.a + (c2.a - c1.a) * t);
    
    return result;
}

// === Accessors ===

video_driver_t* video_get_driver(void) {
    return &video_driver;
}

video_mode_info_t* video_get_mode_info(video_mode_t mode) {
    if (mode >= 0 && mode < sizeof(video_modes) / sizeof(video_modes[0])) {
        return &video_modes[mode];
    }
    return NULL;
}

// === Legacy VGA Fallback ===

// Simple outb implementation
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

int vga_set_mode_13h(void) {
    // Minimal VGA Mode 13h implementation for emergency fallback
    
    outb(0x3C2, 0x63); // Miscellaneous register
    
    // Basic VGA setup - simplified
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F); // All planes
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E); // Chain-4 mode
    outb(0x3CE, 0x05); outb(0x3CF, 0x40); // 256-color mode
    outb(0x3CE, 0x06); outb(0x3CF, 0x05); // Graphics mode
    
    return 1;
}

void vga_put_pixel_fallback(uint16_t x, uint16_t y, uint8_t color) {
    if (x >= 320 || y >= 200) return;
    uint8_t* vga_memory = (uint8_t*)0xA0000;
    vga_memory[y * 320 + x] = color;
}

// === Missing video functions needed by GUI ===

void video_fill_circle_aa(uint32_t cx, uint32_t cy, uint32_t radius, color32_t color) {
    // Fast circle using Bresenham algorithm - much better performance
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    
    // Fill horizontal lines from center outward
    for (int r = 0; r <= (int)radius; r++) {
        int dx = r;
        int dy = 0;
        int err = 0;
        
        while (dx >= dy) {
            // Draw horizontal lines at different heights
            video_draw_line32(cx - dx, cy + dy, cx + dx, cy + dy, color);
            video_draw_line32(cx - dx, cy - dy, cx + dx, cy - dy, color);
            video_draw_line32(cx - dy, cy + dx, cx + dy, cy + dx, color);
            video_draw_line32(cx - dy, cy - dx, cx + dy, cy - dx, color);
            
            if (err <= 0) {
                dy += 1;
                err += 2 * dy + 1;
            }
            if (err > 0) {
                dx -= 1;
                err -= 2 * dx + 1;
            }
        }
    }
}

void video_draw_line_aa(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, color32_t color) {
    // For now, just use regular line drawing
    video_draw_line32(x1, y1, x2, y2, color);
}

void video_draw_circle_aa(uint32_t cx, uint32_t cy, uint32_t radius, color32_t color) {
    // Draw anti-aliased circle outline (simplified without cos/sin)
    // Use Bresenham-style circle algorithm
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    
    while (y >= x) {
        // Draw 8 points for each calculated point
        video_put_pixel32(cx + x, cy + y, color);
        video_put_pixel32(cx - x, cy + y, color);
        video_put_pixel32(cx + x, cy - y, color);
        video_put_pixel32(cx - x, cy - y, color);
        video_put_pixel32(cx + y, cy + x, color);
        video_put_pixel32(cx - y, cy + x, color);
        video_put_pixel32(cx + y, cy - x, color);
        video_put_pixel32(cx - y, cy - x, color);
        
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void video_draw_shadow(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                      uint32_t blur_radius, color32_t shadow_color) {
    // Simple shadow implementation
    for (uint32_t i = 0; i < blur_radius; i++) {
        uint8_t alpha = shadow_color.a - (i * 10);
        if (alpha < 10) alpha = 10;
        
        color32_t layer_color = make_color32(shadow_color.r, shadow_color.g, shadow_color.b, alpha);
        video_draw_rounded_rect(x + i, y + i, width, height, 8, layer_color); // RADIUS_MEDIUM = 8
    }
}

// === Stub implementations for compatibility ===

int vesa_init(void) { return 0; }
int vesa_set_mode(uint16_t mode) { (void)mode; return 0; }
int vesa_detect_modes(void) { return 0; }
void vga_init_fallback(void) { }
void video_shutdown(void) { }
void video_benchmark(void) { }
void video_test_patterns(void) { }
void video_show_info(void) { }

// === DOUBLE BUFFER SUPPORT ДЛЯ MACOS-STYLE TEXT RENDERING ===

void video_copy_buffer(uint32_t* src, uint32_t width, uint32_t height) {
    if (!src) return;
    
    // Быстрое копирование буфера в framebuffer
    // Это должно быть АТОМНОЙ операцией для избежания мигания
    
    for (uint32_t y = 0; y < height && y < video_driver.height; y++) {
        for (uint32_t x = 0; x < width && x < video_driver.width; x++) {
            uint32_t color = src[y * width + x];
            if (color != 0) { // Только непрозрачные пиксели
                video_put_pixel32(x, y, make_color32(
                    (color >> 16) & 0xFF,  // R
                    (color >> 8) & 0xFF,   // G
                    color & 0xFF,          // B
                    (color >> 24) & 0xFF   // A
                ));
            }
        }
    }
}