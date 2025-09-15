#include "../include/usb.h"
#include "../include/pci.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);

static usb_controller_t* usb_controllers[8];
static uint8_t num_controllers = 0;
static usb_device_t* usb_devices = NULL;
static uint8_t next_device_address = 1;

// USB HID Devices
static usb_hid_device_t* usb_mice[4];
static usb_hid_device_t* usb_keyboards[4];
static uint8_t num_mice = 0;
static uint8_t num_keyboards = 0;

// Mouse state for USB mice
static int16_t usb_mouse_x = 0;
static int16_t usb_mouse_y = 0;
static uint8_t usb_mouse_buttons = 0;

// Keyboard state for USB keyboards
static uint8_t usb_key_buffer[256];
static uint8_t usb_key_buffer_head = 0;
static uint8_t usb_key_buffer_tail = 0;

void usb_init(void) {
    terminal_writestring("Initializing USB subsystem...\n");
    
    // Initialize arrays
    for (int i = 0; i < 8; i++) {
        usb_controllers[i] = NULL;
    }
    
    for (int i = 0; i < 4; i++) {
        usb_mice[i] = NULL;
        usb_keyboards[i] = NULL;
    }
    
    // Detect USB controllers
    usb_detect_controllers();
    
    // Initialize HID subsystem
    usb_hid_init();
    usb_mouse_init();
    usb_keyboard_init();
    
    terminal_writestring("USB subsystem initialized\n");
}

void usb_shutdown(void) {
    // Free all devices
    usb_device_t* device = usb_devices;
    while (device) {
        usb_device_t* next = device->next;
        usb_free_device(device);
        device = next;
    }
    
    // Free controllers
    for (int i = 0; i < num_controllers; i++) {
        if (usb_controllers[i]) {
            kfree(usb_controllers[i]);
        }
    }
    
    // Free HID devices
    for (int i = 0; i < 4; i++) {
        if (usb_mice[i]) {
            if (usb_mice[i]->report_buffer) {
                kfree(usb_mice[i]->report_buffer);
            }
            kfree(usb_mice[i]);
        }
        if (usb_keyboards[i]) {
            if (usb_keyboards[i]->report_buffer) {
                kfree(usb_keyboards[i]->report_buffer);
            }
            kfree(usb_keyboards[i]);
        }
    }
}

int usb_detect_controllers(void) {
    terminal_writestring("Scanning for USB controllers...\n");
    
    // Scan PCI bus for USB controllers
    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint16_t vendor_id = pci_read_config_word(bus, device, function, PCI_VENDOR_ID);
                if (vendor_id == 0xFFFF) continue; // No device
                
                uint8_t class_code = pci_read_config_byte(bus, device, function, PCI_CLASS_CODE);
                uint8_t subclass = pci_read_config_byte(bus, device, function, PCI_SUBCLASS);
                
                // Check for USB controller (Class 0x0C, Subclass 0x03)
                if (class_code == 0x0C && subclass == 0x03) {
                    usb_controller_t* controller = (usb_controller_t*)kmalloc(sizeof(usb_controller_t));
                    if (!controller) continue;
                    
                    controller->base_address = pci_read_config_dword(bus, device, function, PCI_BASE_ADDRESS_0) & 0xFFFFFFF0;
                    controller->irq = pci_read_config_byte(bus, device, function, 0x3C); // Interrupt line
                    controller->devices = NULL;
                    
                    // Determine controller type by programming interface
                    uint8_t prog_if = pci_read_config_byte(bus, device, function, PCI_PROG_IF);
                    switch (prog_if) {
                        case 0x00:
                            controller->type = USB_CONTROLLER_UHCI;
                            terminal_writestring("Found UHCI controller\n");
                            break;
                        case 0x10:
                            controller->type = USB_CONTROLLER_OHCI;
                            terminal_writestring("Found OHCI controller\n");
                            break;
                        case 0x20:
                            controller->type = USB_CONTROLLER_EHCI;
                            terminal_writestring("Found EHCI controller\n");
                            break;
                        case 0x30:
                            controller->type = USB_CONTROLLER_XHCI;
                            terminal_writestring("Found XHCI controller\n");
                            break;
                        default:
                            controller->type = USB_CONTROLLER_UHCI;
                            terminal_writestring("Found unknown USB controller (assuming UHCI)\n");
                            break;
                    }
                    
                    usb_register_controller(controller);
                }
            }
        }
    }
    
    return num_controllers;
}

int usb_register_controller(usb_controller_t* controller) {
    if (num_controllers >= 8) {
        return 0;
    }
    
    usb_controllers[num_controllers] = controller;
    num_controllers++;
    
    // Initialize controller (basic init for now)
    // In a real implementation, this would set up the controller registers
    terminal_writestring("USB controller registered\n");
    
    // Enumerate devices on this controller
    usb_enumerate_devices(controller);
    
    return 1;
}

void usb_enumerate_devices(usb_controller_t* controller) {
    (void)controller; // Unused parameter
    
    // Basic device enumeration simulation
    // In a real implementation, this would scan USB ports for connected devices
    
    // For now, simulate finding a USB mouse and keyboard
    static int simulated = 0;
    if (!simulated) {
        terminal_writestring("Simulating USB device enumeration...\n");
        
        // Create simulated USB mouse
        usb_device_t* mouse_device = usb_allocate_device();
        if (mouse_device) {
            mouse_device->address = next_device_address++;
            mouse_device->speed = USB_SPEED_LOW;
            mouse_device->vendor_id = 0x046D; // Logitech
            mouse_device->product_id = 0xC077; // M105 Mouse
            mouse_device->device_class = USB_CLASS_HID;
            mouse_device->device_subclass = USB_HID_SUBCLASS_BOOT;
            mouse_device->device_protocol = USB_HID_PROTOCOL_MOUSE;
            mouse_device->max_packet_size = 8;
            
            // Register as HID device
            usb_hid_register_device(mouse_device);
            terminal_writestring("USB mouse detected and registered\n");
        }
        
        // Create simulated USB keyboard
        usb_device_t* kbd_device = usb_allocate_device();
        if (kbd_device) {
            kbd_device->address = next_device_address++;
            kbd_device->speed = USB_SPEED_LOW;
            kbd_device->vendor_id = 0x04D9; // Holtek
            kbd_device->product_id = 0x1603; // Keyboard
            kbd_device->device_class = USB_CLASS_HID;
            kbd_device->device_subclass = USB_HID_SUBCLASS_BOOT;
            kbd_device->device_protocol = USB_HID_PROTOCOL_KEYBOARD;
            kbd_device->max_packet_size = 8;
            
            // Register as HID device
            usb_hid_register_device(kbd_device);
            terminal_writestring("USB keyboard detected and registered\n");
        }
        
        simulated = 1;
    }
}

usb_device_t* usb_allocate_device(void) {
    usb_device_t* device = (usb_device_t*)kmalloc(sizeof(usb_device_t));
    if (!device) return NULL;
    
    // Initialize device
    device->address = 0;
    device->speed = USB_SPEED_FULL;
    device->port = 0;
    device->vendor_id = 0;
    device->product_id = 0;
    device->device_class = 0;
    device->device_subclass = 0;
    device->device_protocol = 0;
    device->max_packet_size = 64;
    device->configuration = 0;
    device->num_interfaces = 0;
    device->next = NULL;
    device->driver_data = NULL;
    
    // Add to device list
    device->next = usb_devices;
    usb_devices = device;
    
    return device;
}

void usb_free_device(usb_device_t* device) {
    if (!device) return;
    
    // Remove from device list
    if (usb_devices == device) {
        usb_devices = device->next;
    } else {
        usb_device_t* prev = usb_devices;
        while (prev && prev->next != device) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = device->next;
        }
    }
    
    // Free driver data if allocated
    if (device->driver_data) {
        kfree(device->driver_data);
    }
    
    kfree(device);
}

// USB HID Implementation
int usb_hid_init(void) {
    terminal_writestring("Initializing USB HID subsystem...\n");
    return 1;
}

int usb_hid_register_device(usb_device_t* device) {
    if (device->device_class != USB_CLASS_HID) {
        return 0;
    }
    
    usb_hid_device_t* hid = (usb_hid_device_t*)kmalloc(sizeof(usb_hid_device_t));
    if (!hid) return 0;
    
    hid->device = device;
    hid->interface_num = 0;
    hid->endpoint_in = 0x81;  // Typical HID interrupt IN endpoint
    hid->endpoint_out = 0x02; // Typical HID interrupt OUT endpoint
    hid->report_size = 8;
    hid->protocol = device->device_protocol;
    
    // Allocate report buffer
    hid->report_buffer = (uint8_t*)kmalloc(hid->report_size);
    if (!hid->report_buffer) {
        kfree(hid);
        return 0;
    }
    
    // Set appropriate handler based on protocol
    if (device->device_protocol == USB_HID_PROTOCOL_MOUSE) {
        if (num_mice < 4) {
            usb_mice[num_mice] = hid;
            hid->input_handler = usb_mouse_handler;
            num_mice++;
            terminal_writestring("USB HID mouse registered\n");
        } else {
            kfree(hid->report_buffer);
            kfree(hid);
            return 0;
        }
    } else if (device->device_protocol == USB_HID_PROTOCOL_KEYBOARD) {
        if (num_keyboards < 4) {
            usb_keyboards[num_keyboards] = hid;
            hid->input_handler = usb_keyboard_handler;
            num_keyboards++;
            terminal_writestring("USB HID keyboard registered\n");
        } else {
            kfree(hid->report_buffer);
            kfree(hid);
            return 0;
        }
    } else {
        kfree(hid->report_buffer);
        kfree(hid);
        return 0;
    }
    
    device->driver_data = hid;
    return 1;
}

// USB Mouse Implementation
int usb_mouse_init(void) {
    usb_mouse_x = 0;
    usb_mouse_y = 0;
    usb_mouse_buttons = 0;
    return 1;
}

void usb_mouse_handler(usb_hid_device_t* hid, uint8_t* data, uint32_t length) {
    (void)hid; // Unused parameter
    if (length < 3) return;
    
    usb_mouse_report_t* report = (usb_mouse_report_t*)data;
    
    // Update mouse state
    usb_mouse_buttons = report->buttons;
    usb_mouse_x += report->x;
    usb_mouse_y += report->y;
    
    // Keep mouse within screen bounds (assuming 320x200 for now)
    if (usb_mouse_x < 0) usb_mouse_x = 0;
    if (usb_mouse_y < 0) usb_mouse_y = 0;
    if (usb_mouse_x > 319) usb_mouse_x = 319;
    if (usb_mouse_y > 199) usb_mouse_y = 199;
}

int usb_mouse_get_state(int16_t* x, int16_t* y, uint8_t* buttons) {
    if (num_mice == 0) return 0;
    
    *x = usb_mouse_x;
    *y = usb_mouse_y;
    *buttons = usb_mouse_buttons;
    
    return 1;
}

// USB Keyboard Implementation
int usb_keyboard_init(void) {
    usb_key_buffer_head = 0;
    usb_key_buffer_tail = 0;
    return 1;
}

void usb_keyboard_handler(usb_hid_device_t* hid, uint8_t* data, uint32_t length) {
    (void)hid; // Unused parameter
    if (length < 8) return;
    
    usb_keyboard_report_t* report = (usb_keyboard_report_t*)data;
    
    // Simple keycode to ASCII conversion for basic keys
    for (int i = 0; i < 6; i++) {
        uint8_t keycode = report->keycodes[i];
        if (keycode == 0) continue;
        
        char ascii = 0;
        
        // Convert basic letter keycodes (4-29 = A-Z)
        if (keycode >= 4 && keycode <= 29) {
            ascii = 'a' + (keycode - 4);
            if (report->modifiers & 0x22) { // Shift pressed
                ascii = 'A' + (keycode - 4);
            }
        }
        // Convert number keycodes (30-39 = 1-9,0)
        else if (keycode >= 30 && keycode <= 39) {
            if (keycode == 39) {
                ascii = '0';
            } else {
                ascii = '1' + (keycode - 30);
            }
        }
        // Space
        else if (keycode == 44) {
            ascii = ' ';
        }
        // Enter
        else if (keycode == 40) {
            ascii = '\n';
        }
        // Backspace
        else if (keycode == 42) {
            ascii = '\b';
        }
        
        // Add to buffer if valid ASCII
        if (ascii != 0) {
            uint8_t next_head = (usb_key_buffer_head + 1) % 256;
            if (next_head != usb_key_buffer_tail) {
                usb_key_buffer[usb_key_buffer_head] = ascii;
                usb_key_buffer_head = next_head;
            }
        }
    }
}

int usb_keyboard_get_key(void) {
    if (usb_key_buffer_head == usb_key_buffer_tail) {
        return -1; // No key available
    }
    
    uint8_t key = usb_key_buffer[usb_key_buffer_tail];
    usb_key_buffer_tail = (usb_key_buffer_tail + 1) % 256;
    
    return key;
}

int usb_keyboard_available(void) {
    return (usb_key_buffer_head != usb_key_buffer_tail);
}

// Utility Functions
const char* usb_get_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x03: return "HID";
        case 0x08: return "Mass Storage";
        case 0x09: return "Hub";
        case 0x0A: return "CDC Data";
        case 0x0B: return "Smart Card";
        default: return "Unknown";
    }
}

const char* usb_get_speed_name(uint8_t speed) {
    switch (speed) {
        case USB_SPEED_LOW: return "Low Speed (1.5 Mbps)";
        case USB_SPEED_FULL: return "Full Speed (12 Mbps)";
        case USB_SPEED_HIGH: return "High Speed (480 Mbps)";
        default: return "Unknown Speed";
    }
}

void usb_dump_device_info(usb_device_t* device) {
    if (!device) return;
    
    terminal_writestring("USB Device Information:\n");
    terminal_writestring("  Address: ");
    // Simple hex output for address
    char hex_str[4];
    hex_str[0] = '0';
    hex_str[1] = 'x';
    hex_str[2] = '0' + ((device->address >> 4) & 0xF);
    if (hex_str[2] > '9') hex_str[2] = 'A' + (hex_str[2] - '0' - 10);
    hex_str[3] = '0' + (device->address & 0xF);
    if (hex_str[3] > '9') hex_str[3] = 'A' + (hex_str[3] - '0' - 10);
    terminal_writestring(hex_str);
    terminal_writestring("\n");
    
    terminal_writestring("  Class: ");
    terminal_writestring(usb_get_class_name(device->device_class));
    terminal_writestring("\n");
    
    terminal_writestring("  Speed: ");
    terminal_writestring(usb_get_speed_name(device->speed));
    terminal_writestring("\n");
}