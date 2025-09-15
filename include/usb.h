#ifndef USB_H
#define USB_H

#include <stdint.h>

// USB Device Classes
#define USB_CLASS_HID           0x03
#define USB_CLASS_HUB           0x09
#define USB_CLASS_MASS_STORAGE  0x08

// USB HID Subclasses
#define USB_HID_SUBCLASS_BOOT   0x01

// USB HID Protocols
#define USB_HID_PROTOCOL_KEYBOARD  0x01
#define USB_HID_PROTOCOL_MOUSE     0x02

// USB Request Types
#define USB_REQ_GET_STATUS      0x00
#define USB_REQ_CLEAR_FEATURE   0x01
#define USB_REQ_SET_FEATURE     0x03
#define USB_REQ_SET_ADDRESS     0x05
#define USB_REQ_GET_DESCRIPTOR  0x06
#define USB_REQ_SET_DESCRIPTOR  0x07
#define USB_REQ_GET_CONFIG      0x08
#define USB_REQ_SET_CONFIG      0x09
#define USB_REQ_GET_INTERFACE   0x0A
#define USB_REQ_SET_INTERFACE   0x0B

// USB Descriptor Types
#define USB_DESC_DEVICE         0x01
#define USB_DESC_CONFIG         0x02
#define USB_DESC_STRING         0x03
#define USB_DESC_INTERFACE      0x04
#define USB_DESC_ENDPOINT       0x05
#define USB_DESC_HID            0x21
#define USB_DESC_HID_REPORT     0x22

// USB Speed Types
#define USB_SPEED_LOW           0
#define USB_SPEED_FULL          1
#define USB_SPEED_HIGH          2

// USB Endpoint Types
#define USB_EP_CONTROL          0
#define USB_EP_ISOCHRONOUS      1
#define USB_EP_BULK             2
#define USB_EP_INTERRUPT        3

// USB Transfer Direction
#define USB_DIR_OUT             0x00
#define USB_DIR_IN              0x80

// USB HID Report Types
#define USB_HID_REPORT_INPUT    0x01
#define USB_HID_REPORT_OUTPUT   0x02
#define USB_HID_REPORT_FEATURE  0x03

// USB Controller Types
typedef enum {
    USB_CONTROLLER_UHCI,    // USB 1.1 Intel/VIA
    USB_CONTROLLER_OHCI,    // USB 1.1 Compaq/Microsoft
    USB_CONTROLLER_EHCI,    // USB 2.0 Enhanced
    USB_CONTROLLER_XHCI     // USB 3.0+ eXtensible
} usb_controller_type_t;

// USB Device Descriptor
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} usb_device_descriptor_t;

// USB Configuration Descriptor
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} usb_config_descriptor_t;

// USB Interface Descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} usb_interface_descriptor_t;

// USB Endpoint Descriptor
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} usb_endpoint_descriptor_t;

// USB HID Descriptor
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bDescriptorType2;
    uint16_t wDescriptorLength;
} usb_hid_descriptor_t;

// USB Setup Packet
typedef struct __attribute__((packed)) {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

// USB Transfer Request
typedef struct {
    uint8_t  device_address;
    uint8_t  endpoint;
    uint8_t  type;
    uint8_t  direction;
    void*    buffer;
    uint32_t length;
    uint32_t actual_length;
    int      status;
    void (*callback)(void* request);
    void*    user_data;
} usb_transfer_t;

// USB Device
typedef struct usb_device {
    uint8_t  address;
    uint8_t  speed;
    uint8_t  port;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t  device_class;
    uint8_t  device_subclass;
    uint8_t  device_protocol;
    uint8_t  max_packet_size;
    uint8_t  configuration;
    uint8_t  num_interfaces;
    
    usb_device_descriptor_t device_desc;
    usb_config_descriptor_t config_desc;
    
    struct usb_device* next;
    void* driver_data;
} usb_device_t;

// Forward declarations
typedef struct usb_controller usb_controller_t;
typedef struct usb_hid_device usb_hid_device_t;

// USB Controller
typedef struct usb_controller {
    usb_controller_type_t type;
    uint32_t base_address;
    uint8_t  irq;
    uint8_t  num_ports;
    usb_device_t* devices;
    
    // Controller-specific operations
    int (*init)(usb_controller_t* controller);
    int (*reset_port)(usb_controller_t* controller, uint8_t port);
    int (*enable_port)(usb_controller_t* controller, uint8_t port);
    int (*control_transfer)(usb_controller_t* controller, usb_transfer_t* transfer);
    int (*interrupt_transfer)(usb_controller_t* controller, usb_transfer_t* transfer);
    int (*bulk_transfer)(usb_controller_t* controller, usb_transfer_t* transfer);
} usb_controller_t;

// USB HID Device
typedef struct usb_hid_device {
    usb_device_t* device;
    uint8_t interface_num;
    uint8_t endpoint_in;
    uint8_t endpoint_out;
    uint8_t report_size;
    uint8_t protocol;
    uint8_t* report_buffer;
    
    // HID-specific callbacks
    void (*input_handler)(usb_hid_device_t* hid, uint8_t* data, uint32_t length);
} usb_hid_device_t;

// USB Mouse Report (Boot Protocol)
typedef struct __attribute__((packed)) {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} usb_mouse_report_t;

// USB Keyboard Report (Boot Protocol)
typedef struct __attribute__((packed)) {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keycodes[6];
} usb_keyboard_report_t;

// USB Core Functions
void usb_init(void);
void usb_shutdown(void);
int usb_detect_controllers(void);
int usb_register_controller(usb_controller_t* controller);
void usb_enumerate_devices(usb_controller_t* controller);

// USB Device Management
usb_device_t* usb_allocate_device(void);
void usb_free_device(usb_device_t* device);
int usb_configure_device(usb_device_t* device);
int usb_set_address(usb_device_t* device, uint8_t address);

// USB Transfer Functions
int usb_control_transfer(usb_device_t* device, usb_setup_packet_t* setup, void* data);
int usb_interrupt_transfer(usb_device_t* device, uint8_t endpoint, void* data, uint32_t length);
int usb_bulk_transfer(usb_device_t* device, uint8_t endpoint, void* data, uint32_t length);

// USB HID Functions
int usb_hid_init(void);
int usb_hid_register_device(usb_device_t* device);
int usb_hid_set_protocol(usb_hid_device_t* hid, uint8_t protocol);
int usb_hid_get_report(usb_hid_device_t* hid, uint8_t type, uint8_t id, void* data, uint32_t length);
int usb_hid_set_report(usb_hid_device_t* hid, uint8_t type, uint8_t id, void* data, uint32_t length);

// USB Mouse Functions
int usb_mouse_init(void);
void usb_mouse_handler(usb_hid_device_t* hid, uint8_t* data, uint32_t length);
int usb_mouse_get_state(int16_t* x, int16_t* y, uint8_t* buttons);

// USB Keyboard Functions
int usb_keyboard_init(void);
void usb_keyboard_handler(usb_hid_device_t* hid, uint8_t* data, uint32_t length);
int usb_keyboard_get_key(void);
int usb_keyboard_available(void);

// USB Utility Functions
const char* usb_get_class_name(uint8_t class_code);
const char* usb_get_speed_name(uint8_t speed);
void usb_dump_device_info(usb_device_t* device);

#endif