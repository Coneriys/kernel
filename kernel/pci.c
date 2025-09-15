#include "../include/pci.h"

extern void terminal_writestring(const char* data);

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static pci_device_t detected_devices[256];
static int device_count = 0;

void pci_init(void) {
    terminal_writestring("Initializing PCI bus...\n");
    device_count = 0;
    pci_scan_bus();
    
    char device_str[64];
    terminal_writestring("PCI scan complete. Found ");
    // Simple int to string
    if (device_count == 0) {
        terminal_writestring("0");
    } else {
        int temp = device_count;
        char nums[16];
        int pos = 0;
        while (temp > 0) {
            nums[pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = pos-1; i >= 0; i--) {
            device_str[0] = nums[i];
            device_str[1] = '\0';
            terminal_writestring(device_str);
        }
    }
    terminal_writestring(" devices\n");
}

uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | 
                      ((uint32_t)function << 8) | (offset & 0xFC);
    
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t dword = pci_read_config_dword(bus, device, function, offset & 0xFC);
    return (uint16_t)((dword >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t dword = pci_read_config_dword(bus, device, function, offset & 0xFC);
    return (uint8_t)((dword >> ((offset & 3) * 8)) & 0xFF);
}

void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (1 << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | 
                      ((uint32_t)function << 8) | (offset & 0xFC);
    
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev) {
    for (int i = 0; i < device_count; i++) {
        if (detected_devices[i].vendor_id == vendor_id && 
            detected_devices[i].device_id == device_id) {
            *dev = detected_devices[i];
            return 1;
        }
    }
    return 0;
}

static void pci_check_device(uint8_t bus, uint8_t device) {
    uint16_t vendor_id = pci_read_config_word(bus, device, 0, PCI_VENDOR_ID);
    
    if (vendor_id == 0xFFFF) {
        return; // No device
    }
    
    if (device_count >= 256) {
        return; // Too many devices
    }
    
    pci_device_t* dev = &detected_devices[device_count];
    dev->vendor_id = vendor_id;
    dev->device_id = pci_read_config_word(bus, device, 0, PCI_DEVICE_ID);
    dev->bus = bus;
    dev->device = device;
    dev->function = 0;
    dev->class_code = pci_read_config_byte(bus, device, 0, PCI_CLASS_CODE);
    dev->subclass = pci_read_config_byte(bus, device, 0, PCI_SUBCLASS);
    
    // Read base addresses
    for (int i = 0; i < 6; i++) {
        dev->base_addresses[i] = pci_read_config_dword(bus, device, 0, PCI_BASE_ADDRESS_0 + (i * 4));
    }
    
    device_count++;
    
    // Check for multifunction device
    uint8_t header_type = pci_read_config_byte(bus, device, 0, PCI_HEADER_TYPE);
    if (header_type & 0x80) {
        // Multifunction device - check other functions
        for (int func = 1; func < 8; func++) {
            uint16_t func_vendor = pci_read_config_word(bus, device, func, PCI_VENDOR_ID);
            if (func_vendor != 0xFFFF && device_count < 256) {
                pci_device_t* func_dev = &detected_devices[device_count];
                func_dev->vendor_id = func_vendor;
                func_dev->device_id = pci_read_config_word(bus, device, func, PCI_DEVICE_ID);
                func_dev->bus = bus;
                func_dev->device = device;
                func_dev->function = func;
                func_dev->class_code = pci_read_config_byte(bus, device, func, PCI_CLASS_CODE);
                func_dev->subclass = pci_read_config_byte(bus, device, func, PCI_SUBCLASS);
                
                for (int i = 0; i < 6; i++) {
                    func_dev->base_addresses[i] = pci_read_config_dword(bus, device, func, PCI_BASE_ADDRESS_0 + (i * 4));
                }
                
                device_count++;
            }
        }
    }
}

int pci_scan_bus(void) {
    // Scan PCI bus 0 for devices
    for (uint8_t device = 0; device < 32; device++) {
        pci_check_device(0, device);
    }
    
    return device_count;
}