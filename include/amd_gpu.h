#ifndef AMD_GPU_H
#define AMD_GPU_H

#include <stdint.h>
#include "pci.h"

// AMD GPU Device IDs (comprehensive list)
#define AMD_VENDOR_ID 0x1002

// RX 6000 Series (RDNA 2)
#define AMD_RADEON_RX6900XT 0x73BF
#define AMD_RADEON_RX6800XT 0x73A2
#define AMD_RADEON_RX6800   0x73AB
#define AMD_RADEON_RX6700XT 0x73DF
#define AMD_RADEON_RX6600XT 0x73FF
#define AMD_RADEON_RX6600   0x7340
#define AMD_RADEON_RX6500XT 0x7341
#define AMD_RADEON_RX6400   0x7342

// RX 5000 Series (RDNA 1)
#define AMD_RADEON_RX5700XT 0x731F
#define AMD_RADEON_RX5700   0x7318
#define AMD_RADEON_RX5600XT 0x731E
#define AMD_RADEON_RX5500XT 0x7338
#define AMD_RADEON_RX5500   0x7339
#define AMD_RADEON_RX5300   0x7347

// RX 500 Series (Polaris)
#define AMD_RADEON_RX580    0x67DF
#define AMD_RADEON_RX570    0x67EF
#define AMD_RADEON_RX560    0x67FF

// Older generations
#define AMD_RADEON_R9_290X  0x67B0
#define AMD_RADEON_R7_260X  0x6658
#define AMD_RADEON_HD7970   0x6798
#define AMD_RADEON_HD7870   0x6818
#define AMD_RADEON_HD6970   0x6719
#define AMD_RADEON_HD5870   0x6899

// Legacy compatibility names
#define RX_6600_XT_DEVICE_ID AMD_RADEON_RX6600XT
#define RX_6700_XT_DEVICE_ID AMD_RADEON_RX6700XT
#define RX_6800_XT_DEVICE_ID AMD_RADEON_RX6800XT

// AMD GPU Register Offsets (MMIO)
#define AMD_SURFACE_CNTL               0x0B00
#define AMD_CRTC_GEN_CNTL              0x0050
#define AMD_CRTC_EXT_CNTL              0x0054
#define AMD_DAC_CNTL                   0x0058
#define AMD_CRTC_H_TOTAL_DISP          0x0200
#define AMD_CRTC_H_SYNC_STRT_WID       0x0204
#define AMD_CRTC_V_TOTAL_DISP          0x0208
#define AMD_CRTC_V_SYNC_STRT_WID       0x020C
#define AMD_CRTC_OFFSET                0x0224
#define AMD_CRTC_OFFSET_CNTL           0x0228
#define AMD_CRTC_PITCH                 0x022C

// Memory Controller Registers  
#define AMD_MC_FB_LOCATION             0x148
#define AMD_MC_AGP_LOCATION            0x14C
#define AMD_DISPLAY_BASE_ADDR          0x23C
#define AMD_CRTC2_DISPLAY_BASE_ADDR    0x33C

// Additional GPU Registers
#define AMD_SURFACE0_INFO              0x0B0C
#define AMD_SURFACE0_LOWER_BOUND       0x0B04
#define AMD_SURFACE0_UPPER_BOUND       0x0B08
#define AMD_SURFACE1_INFO              0x0B1C
#define AMD_SURFACE2_INFO              0x0B2C
#define AMD_SURFACE3_INFO              0x0B3C
#define AMD_CONFIG_APER_SIZE           0x0108
#define AMD_MM_INDEX                   0x0000
#define AMD_MM_DATA                    0x0004

// Control Bits
#define AMD_CRTC_EXT_DISP_EN           0x01000000
#define AMD_CRTC_EN                    0x02000000
#define AMD_CRTC_DISP_REQ_EN_B         0x04000000

// Forward declare color types from video.h
typedef struct {
    uint8_t b, g, r, a;
} color_t;

// AMD GPU registers and capabilities
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t base_address;
    uint32_t memory_size;
    uint8_t initialized;
    char name[64];
} amd_gpu_t;

// Function declarations
void amd_gpu_init(void);
int amd_gpu_detect_device(pci_device_t* device);
int amd_gpu_set_mode(uint32_t width, uint32_t height, uint32_t bpp);
void amd_gpu_set_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t* amd_gpu_get_framebuffer(void);

#endif // AMD_GPU_H