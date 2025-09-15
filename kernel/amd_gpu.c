#include "../include/video.h"
#include "../include/pci.h"
#include "../include/amd_gpu.h"

extern void terminal_writestring(const char* data);

static pci_device_t amd_gpu_device;
static int amd_gpu_detected = 0;
static uint32_t amd_framebuffer_addr = 0;
static uint32_t amd_framebuffer_size = 0;
static uint32_t amd_mmio_base = 0;

static inline void mmio_write32(uint32_t addr, uint32_t value) {
    *(volatile uint32_t*)addr = value;
}

static inline uint32_t mmio_read32(uint32_t addr) {
    return *(volatile uint32_t*)addr;
}

static int is_amd_gpu_device(uint16_t device_id) {
    switch (device_id) {
        // RX 6000 Series (RDNA 2)
        case AMD_RADEON_RX6900XT:
        case AMD_RADEON_RX6800XT:
        case AMD_RADEON_RX6800:
        case AMD_RADEON_RX6700XT:
        case AMD_RADEON_RX6600XT:
        case AMD_RADEON_RX6600:
        case AMD_RADEON_RX6500XT:
        case AMD_RADEON_RX6400:
        
        // RX 5000 Series (RDNA 1)
        case AMD_RADEON_RX5700XT:
        case AMD_RADEON_RX5700:
        case AMD_RADEON_RX5600XT:
        case AMD_RADEON_RX5500XT:
        case AMD_RADEON_RX5500:
        case AMD_RADEON_RX5300:
        
        // RX 500 Series (Polaris)
        case AMD_RADEON_RX580:
        case AMD_RADEON_RX570:
        case AMD_RADEON_RX560:
        
        // Older generations
        case AMD_RADEON_R9_290X:
        case AMD_RADEON_R7_260X:
        case AMD_RADEON_HD7970:
        case AMD_RADEON_HD7870:
        case AMD_RADEON_HD6970:
        case AMD_RADEON_HD5870:
            return 1;
        default:
            return 0;
    }
}

int detect_amd_gpu(void) {
    if (amd_gpu_detected) {
        return 1;
    }
    
    // Scan for AMD GPUs
    for (int i = 0; i < 256; i++) {
        pci_device_t dev;
        
        // Try to find any AMD GPU
        if (pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6900XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6800XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6800, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6700XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6600XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6600, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6500XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX6400, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX5700XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX5700, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX5600XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX5500XT, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX5500, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX5300, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX580, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX570, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_RX560, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_R9_290X, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_R7_260X, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_HD7970, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_HD7870, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_HD6970, &dev) ||
            pci_find_device(AMD_VENDOR_ID, AMD_RADEON_HD5870, &dev)) {
            
            if (dev.class_code == PCI_CLASS_DISPLAY) {
                amd_gpu_device = dev;
                amd_gpu_detected = 1;
                
                // Get framebuffer address (usually BAR0)
                amd_framebuffer_addr = dev.base_addresses[0] & 0xFFFFFFF0;
                
                // Get MMIO base (usually BAR2)
                amd_mmio_base = dev.base_addresses[2] & 0xFFFFFFF0;
                
                terminal_writestring("AMD GPU detected: ");
                switch (dev.device_id) {
                    // RX 6000 Series (RDNA 2)
                    case AMD_RADEON_RX6900XT:
                        terminal_writestring("Radeon RX 6900 XT\n");
                        break;
                    case AMD_RADEON_RX6800XT:
                        terminal_writestring("Radeon RX 6800 XT\n");
                        break;
                    case AMD_RADEON_RX6800:
                        terminal_writestring("Radeon RX 6800\n");
                        break;
                    case AMD_RADEON_RX6700XT:
                        terminal_writestring("Radeon RX 6700 XT\n");
                        break;
                    case AMD_RADEON_RX6600XT:
                        terminal_writestring("Radeon RX 6600 XT\n");
                        break;
                    case AMD_RADEON_RX6600:
                        terminal_writestring("Radeon RX 6600\n");
                        break;
                    case AMD_RADEON_RX6500XT:
                        terminal_writestring("Radeon RX 6500 XT\n");
                        break;
                    case AMD_RADEON_RX6400:
                        terminal_writestring("Radeon RX 6400\n");
                        break;
                    
                    // RX 5000 Series (RDNA 1)
                    case AMD_RADEON_RX5700XT:
                        terminal_writestring("Radeon RX 5700 XT\n");
                        break;
                    case AMD_RADEON_RX5700:
                        terminal_writestring("Radeon RX 5700\n");
                        break;
                    case AMD_RADEON_RX5600XT:
                        terminal_writestring("Radeon RX 5600 XT\n");
                        break;
                    case AMD_RADEON_RX5500XT:
                        terminal_writestring("Radeon RX 5500 XT\n");
                        break;
                    case AMD_RADEON_RX5500:
                        terminal_writestring("Radeon RX 5500\n");
                        break;
                    case AMD_RADEON_RX5300:
                        terminal_writestring("Radeon RX 5300\n");
                        break;
                    
                    // RX 500 Series (Polaris)
                    case AMD_RADEON_RX580:
                        terminal_writestring("Radeon RX 580\n");
                        break;
                    case AMD_RADEON_RX570:
                        terminal_writestring("Radeon RX 570\n");
                        break;
                    case AMD_RADEON_RX560:
                        terminal_writestring("Radeon RX 560\n");
                        break;
                    
                    // Older generations
                    case AMD_RADEON_R9_290X:
                        terminal_writestring("Radeon R9 290X\n");
                        break;
                    case AMD_RADEON_R7_260X:
                        terminal_writestring("Radeon R7 260X\n");
                        break;
                    case AMD_RADEON_HD7970:
                        terminal_writestring("Radeon HD 7970\n");
                        break;
                    case AMD_RADEON_HD7870:
                        terminal_writestring("Radeon HD 7870\n");
                        break;
                    case AMD_RADEON_HD6970:
                        terminal_writestring("Radeon HD 6970\n");
                        break;
                    case AMD_RADEON_HD5870:
                        terminal_writestring("Radeon HD 5870\n");
                        break;
                    default:
                        terminal_writestring("Unknown AMD GPU\n");
                        break;
                }
                
                return 1;
            }
        }
    }
    
    return 0;
}

void amd_gpu_init(void) {
    if (!detect_amd_gpu()) {
        terminal_writestring("No AMD GPU detected\n");
        return;
    }
    
    terminal_writestring("Initializing AMD GPU driver...\n");
    
    // Enable memory and I/O access
    uint16_t command = pci_read_config_word(amd_gpu_device.bus, amd_gpu_device.device, 
                                           amd_gpu_device.function, PCI_COMMAND);
    command |= 0x06; // Enable memory space and bus mastering
    pci_write_config_dword(amd_gpu_device.bus, amd_gpu_device.device, 
                          amd_gpu_device.function, PCI_COMMAND, command);
    
    // Basic initialization sequence for AMD GPUs
    if (amd_mmio_base != 0) {
        // Reset GPU state
        mmio_write32(amd_mmio_base + AMD_SURFACE_CNTL, 0);
        
        // Set up basic display controller
        mmio_write32(amd_mmio_base + AMD_CRTC_GEN_CNTL, 0x00000100);
        mmio_write32(amd_mmio_base + AMD_CRTC_EXT_CNTL, 0);
        
        // Configure surface 0 for framebuffer
        if (amd_framebuffer_addr != 0) {
            mmio_write32(amd_mmio_base + AMD_SURFACE0_LOWER_BOUND, amd_framebuffer_addr);
            mmio_write32(amd_mmio_base + AMD_SURFACE0_UPPER_BOUND, amd_framebuffer_addr + 0x400000);
            mmio_write32(amd_mmio_base + AMD_SURFACE0_INFO, 0x00000000);
        }
    }
    
    // Estimate framebuffer size (conservative 4MB)
    amd_framebuffer_size = 4 * 1024 * 1024;
    
    terminal_writestring("AMD GPU driver initialized\n");
}

int amd_set_mode(uint32_t width, uint32_t height, uint32_t depth) {
    if (!amd_gpu_detected || amd_mmio_base == 0) {
        return 0;
    }
    
    // Basic mode setting for AMD GPUs
    // This is a simplified implementation
    
    uint32_t h_total = width + 160;  // Add blanking
    uint32_t v_total = height + 45;  // Add blanking
    
    // Set CRTC timing
    mmio_write32(amd_mmio_base + AMD_CRTC_H_TOTAL_DISP, (h_total << 16) | (width - 1));
    mmio_write32(amd_mmio_base + AMD_CRTC_V_TOTAL_DISP, (v_total << 16) | (height - 1));
    
    // Enable display
    uint32_t crtc_gen_cntl = mmio_read32(amd_mmio_base + AMD_CRTC_GEN_CNTL);
    crtc_gen_cntl |= 0x00000100; // Enable CRTC
    mmio_write32(amd_mmio_base + AMD_CRTC_GEN_CNTL, crtc_gen_cntl);
    
    return 1;
}

void amd_put_pixel(uint32_t x, uint32_t y, color_t color) {
    if (!amd_gpu_detected || amd_framebuffer_addr == 0) {
        return;
    }
    
    // Assume 32-bit color depth for simplicity
    uint32_t offset = (y * 1024 + x) * 4; // Assume 1024 width for now
    uint32_t pixel = (color.r << 16) | (color.g << 8) | color.b;
    
    if (offset < amd_framebuffer_size) {
        *(volatile uint32_t*)(amd_framebuffer_addr + offset) = pixel;
    }
}

void amd_clear_screen(color_t color) {
    if (!amd_gpu_detected || amd_framebuffer_addr == 0) {
        return;
    }
    
    uint32_t pixel = (color.r << 16) | (color.g << 8) | color.b;
    
    // Clear entire framebuffer
    for (uint32_t i = 0; i < amd_framebuffer_size / 4; i++) {
        *(volatile uint32_t*)(amd_framebuffer_addr + i * 4) = pixel;
    }
}

int amd_get_framebuffer_info(uint32_t* addr, uint32_t* size) {
    if (!amd_gpu_detected) {
        return 0;
    }
    
    if (addr) *addr = amd_framebuffer_addr;
    if (size) *size = amd_framebuffer_size;
    
    return 1;
}