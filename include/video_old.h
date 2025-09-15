#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include <stddef.h>

// VGA Text Mode
#define VGA_TEXT_MEMORY    0xB8000
#define VGA_TEXT_WIDTH     80
#define VGA_TEXT_HEIGHT    25

// VGA Graphics Mode (Mode 13h - 320x200x256)
#define VGA_GRAPHICS_MEMORY 0xA0000
#define VGA_GRAPHICS_WIDTH  320
#define VGA_GRAPHICS_HEIGHT 200

// VESA modes
#define VESA_MODE_640x480x16   0x111
#define VESA_MODE_800x600x16   0x114  
#define VESA_MODE_1024x768x16  0x117
#define VESA_MODE_1280x720x16  0x119  // 720p mode
#define VESA_MODE_1280x1024x16 0x11A
#define VESA_MODE_1600x1200x16 0x11C
#define VESA_MODE_640x480x24   0x112
#define VESA_MODE_800x600x24   0x115
#define VESA_MODE_1024x768x24  0x118
#define VESA_MODE_1280x720x24  0x11F  // 720p 24-bit mode
#define VESA_MODE_1280x1024x24 0x11B
#define VESA_MODE_1600x1200x24 0x11D

// QEMU/KVM specific modes (custom)
#define QEMU_MODE_1280x720x32  0x180  // QEMU 720p 32-bit
#define QEMU_MODE_1920x1080x32 0x181  // QEMU 1080p 32-bit

// Video modes
typedef enum {
    VIDEO_MODE_TEXT,
    VIDEO_MODE_VGA_320x200,
    VIDEO_MODE_VESA_640x480,
    VIDEO_MODE_VESA_800x600,
    VIDEO_MODE_VESA_1024x768,
    VIDEO_MODE_VESA_1280x720,    // 720p HD mode
    VIDEO_MODE_VESA_1280x1024,
    VIDEO_MODE_VESA_1600x1200,
    VIDEO_MODE_QEMU_1280x720,    // QEMU optimized 720p
    VIDEO_MODE_QEMU_1920x1080    // QEMU optimized 1080p
} video_mode_t;

// Color structure
typedef struct {
    uint8_t r, g, b;
} color_t;

// Video driver structure
typedef struct {
    video_mode_t current_mode;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t framebuffer;
    uint32_t pitch;
} video_driver_t;

// Basic video functions
void video_init(void);
int video_set_mode(video_mode_t mode);
void video_clear_screen(color_t color);
void video_put_pixel(uint32_t x, uint32_t y, color_t color);
color_t video_get_pixel(uint32_t x, uint32_t y);

// VGA specific functions
void vga_init(void);
void vga_set_mode_13h(void);
void vga_put_pixel_13h(uint16_t x, uint16_t y, uint8_t color);

// VESA specific functions
void vesa_init(void);
int vesa_set_mode(uint16_t mode);
void vesa_put_pixel(uint32_t x, uint32_t y, color_t color);

// Drawing functions
void draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, color_t color);
void draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);
void draw_filled_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);

// Utility functions
color_t make_color(uint8_t r, uint8_t g, uint8_t b);
video_driver_t* get_video_driver(void);
video_mode_t video_detect_best_mode(void);
int video_get_mode_resolution(video_mode_t mode, uint32_t* width, uint32_t* height);

// Hardware detection
int detect_vga(void);
int detect_vesa(void);
int detect_amd_gpu(void);

// AMD GPU support
void amd_gpu_init(void);
int amd_set_mode(uint32_t width, uint32_t height, uint32_t depth);
void amd_put_pixel(uint32_t x, uint32_t y, color_t color);
void amd_clear_screen(color_t color);
int amd_get_framebuffer_info(uint32_t* addr, uint32_t* size);

// QEMU/KVM virtualization support
void qemu_video_init(void);
int qemu_detect_device(void);
int qemu_set_mode(uint32_t width, uint32_t height, uint32_t depth);
void qemu_put_pixel(uint32_t x, uint32_t y, color_t color);
void qemu_clear_screen(color_t color);
int qemu_get_framebuffer_info(uint32_t* addr, uint32_t* size);
void qemu_enable_acceleration(void);

// VirtIO GPU support (QEMU)
#define VIRTIO_GPU_VENDOR_ID   0x1AF4
#define VIRTIO_GPU_DEVICE_ID   0x1050
#define QEMU_VGA_VENDOR_ID     0x1234
#define QEMU_VGA_DEVICE_ID     0x1111

// Bochs/QEMU VGA registers
#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF
#define VBE_DISPI_INDEX_ID     0x0
#define VBE_DISPI_INDEX_XRES   0x1
#define VBE_DISPI_INDEX_YRES   0x2
#define VBE_DISPI_INDEX_BPP    0x3
#define VBE_DISPI_INDEX_ENABLE 0x4
#define VBE_DISPI_INDEX_BANK   0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH  0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET    0x8
#define VBE_DISPI_INDEX_Y_OFFSET    0x9

#define VBE_DISPI_DISABLED     0x00
#define VBE_DISPI_ENABLED      0x01
#define VBE_DISPI_LFB_ENABLED  0x40

// AMD GPU device IDs (common AMD/ATI cards)
#define AMD_VENDOR_ID          0x1002

// RX 6000 Series (RDNA 2)
#define AMD_RADEON_RX6900XT    0x73AF
#define AMD_RADEON_RX6800XT    0x73BF
#define AMD_RADEON_RX6800      0x73AB
#define AMD_RADEON_RX6700XT    0x73DF
#define AMD_RADEON_RX6600XT    0x73EF
#define AMD_RADEON_RX6600      0x73FF
#define AMD_RADEON_RX6500XT    0x743F
#define AMD_RADEON_RX6400      0x7434

// RX 5000 Series (RDNA 1)
#define AMD_RADEON_RX5700XT    0x731F
#define AMD_RADEON_RX5700      0x7318
#define AMD_RADEON_RX5600XT    0x731B
#define AMD_RADEON_RX5500XT    0x7340
#define AMD_RADEON_RX5500      0x7341
#define AMD_RADEON_RX5300      0x7347

// RX 500 Series (Polaris)
#define AMD_RADEON_RX580       0x67DF
#define AMD_RADEON_RX570       0x67B0
#define AMD_RADEON_RX560       0x67EF

// Older generations
#define AMD_RADEON_R9_290X     0x67B1
#define AMD_RADEON_R7_260X     0x6658
#define AMD_RADEON_HD7970      0x6798
#define AMD_RADEON_HD7870      0x6818
#define AMD_RADEON_HD6970      0x6719
#define AMD_RADEON_HD5870      0x6898

// AMD GPU registers
#define AMD_CONFIG_MEMSIZE     0x5428
#define AMD_CRTC_GEN_CNTL      0x0050
#define AMD_CRTC_EXT_CNTL      0x0054
#define AMD_CRTC_H_TOTAL_DISP  0x0200
#define AMD_CRTC_V_TOTAL_DISP  0x0208
#define AMD_SURFACE_CNTL       0x0B00
#define AMD_SURFACE0_INFO      0x0B0C
#define AMD_SURFACE0_LOWER_BOUND 0x0B04
#define AMD_SURFACE0_UPPER_BOUND 0x0B08

#endif