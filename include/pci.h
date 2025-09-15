#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI Configuration Space
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// PCI Header fields
#define PCI_VENDOR_ID      0x00
#define PCI_DEVICE_ID      0x02
#define PCI_COMMAND        0x04
#define PCI_STATUS         0x06
#define PCI_CLASS_CODE     0x0B
#define PCI_SUBCLASS       0x0A
#define PCI_PROG_IF        0x09
#define PCI_REVISION_ID    0x08
#define PCI_HEADER_TYPE    0x0E
#define PCI_BASE_ADDRESS_0 0x10
#define PCI_BASE_ADDRESS_1 0x14
#define PCI_BASE_ADDRESS_2 0x18
#define PCI_BASE_ADDRESS_3 0x1C
#define PCI_BASE_ADDRESS_4 0x20
#define PCI_BASE_ADDRESS_5 0x24

// PCI Device Classes
#define PCI_CLASS_DISPLAY  0x03

// PCI Device structure
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint32_t base_addresses[6];
    uint8_t class_code;
    uint8_t subclass;
} pci_device_t;

// PCI Functions
void pci_init(void);
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint8_t pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev);
int pci_scan_bus(void);

#endif