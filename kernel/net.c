#include "../include/net.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);

static net_buffer_t net_buffers[NET_MAX_BUFFERS];
static net_interface_t interface;

static void net_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void net_init(void) {
    // Initialize buffer pool
    for (int i = 0; i < NET_MAX_BUFFERS; i++) {
        net_buffers[i].in_use = 0;
        net_buffers[i].length = 0;
    }
    
    // Initialize interface
    interface.active = 0;
    net_strcpy(interface.name, "eth0");
    
    // Set default MAC (would normally be read from hardware)
    interface.mac.addr[0] = 0x52;
    interface.mac.addr[1] = 0x54;
    interface.mac.addr[2] = 0x00;
    interface.mac.addr[3] = 0x12;
    interface.mac.addr[4] = 0x34;
    interface.mac.addr[5] = 0x56;
    
    terminal_writestring("Network stack initialized\n");
}

net_buffer_t* net_alloc_buffer(void) {
    for (int i = 0; i < NET_MAX_BUFFERS; i++) {
        if (!net_buffers[i].in_use) {
            net_buffers[i].in_use = 1;
            net_buffers[i].length = 0;
            return &net_buffers[i];
        }
    }
    return NULL; // No free buffers
}

void net_free_buffer(net_buffer_t* buffer) {
    if (buffer) {
        buffer->in_use = 0;
        buffer->length = 0;
    }
}

void net_send_packet(net_buffer_t* buffer) {
    // TODO: This would send to actual network hardware
    // For now, just simulate sending
    (void)buffer;
}

void net_receive_packet(uint8_t* data, size_t length) {
    if (length < sizeof(eth_header_t)) {
        return; // Too small to be valid
    }
    
    net_buffer_t* buffer = net_alloc_buffer();
    if (!buffer) {
        return; // No free buffers
    }
    
    // Copy packet data
    for (size_t i = 0; i < length && i < NET_BUFFER_SIZE; i++) {
        buffer->data[i] = data[i];
    }
    buffer->length = length;
    
    // Parse Ethernet header
    eth_header_t* eth = (eth_header_t*)buffer->data;
    uint16_t type = ntohs(eth->type);
    
    // Handle different protocols
    switch (type) {
        case ETH_TYPE_IP:
            // ip_handle_packet(buffer, sizeof(eth_header_t));
            break;
        case ETH_TYPE_ARP:
            // arp_handle_packet(buffer, sizeof(eth_header_t));
            break;
        default:
            // Unknown protocol
            break;
    }
    
    net_free_buffer(buffer);
}

void net_set_interface(mac_addr_t mac, ip_addr_t ip, ip_addr_t netmask, ip_addr_t gateway) {
    mac_copy(&interface.mac, &mac);
    ip_copy(&interface.ip, &ip);
    ip_copy(&interface.netmask, &netmask);
    ip_copy(&interface.gateway, &gateway);
    interface.active = 1;
}

net_interface_t* net_get_interface(void) {
    return &interface;
}

int mac_compare(mac_addr_t* a, mac_addr_t* b) {
    for (int i = 0; i < 6; i++) {
        if (a->addr[i] != b->addr[i]) {
            return 0;
        }
    }
    return 1;
}

int ip_compare(ip_addr_t* a, ip_addr_t* b) {
    for (int i = 0; i < 4; i++) {
        if (a->addr[i] != b->addr[i]) {
            return 0;
        }
    }
    return 1;
}

void mac_copy(mac_addr_t* dest, mac_addr_t* src) {
    for (int i = 0; i < 6; i++) {
        dest->addr[i] = src->addr[i];
    }
}

void ip_copy(ip_addr_t* dest, ip_addr_t* src) {
    for (int i = 0; i < 4; i++) {
        dest->addr[i] = src->addr[i];
    }
}

char* mac_to_string(mac_addr_t* mac) {
    static char buffer[18];
    // Simple hex to string conversion
    const char hex[] = "0123456789ABCDEF";
    
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        if (i > 0) {
            buffer[pos++] = ':';
        }
        buffer[pos++] = hex[(mac->addr[i] >> 4) & 0xF];
        buffer[pos++] = hex[mac->addr[i] & 0xF];
    }
    buffer[pos] = '\0';
    
    return buffer;
}

char* ip_to_string(ip_addr_t* ip) {
    static char buffer[16];
    
    // Simple decimal conversion
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            buffer[pos++] = '.';
        }
        
        uint8_t octet = ip->addr[i];
        if (octet >= 100) {
            buffer[pos++] = '0' + (octet / 100);
            octet %= 100;
        }
        if (octet >= 10 || ip->addr[i] >= 100) {
            buffer[pos++] = '0' + (octet / 10);
            octet %= 10;
        }
        buffer[pos++] = '0' + octet;
    }
    buffer[pos] = '\0';
    
    return buffer;
}

uint32_t get_local_ip(void) {
    net_interface_t* iface = net_get_interface();
    if (iface && iface->active) {
        return (uint32_t)(iface->ip.addr[0] << 24) |
               (uint32_t)(iface->ip.addr[1] << 16) |
               (uint32_t)(iface->ip.addr[2] << 8) |
               (uint32_t)(iface->ip.addr[3]);
    }
    return 0;
}