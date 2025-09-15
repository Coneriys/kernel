#ifndef UDP_H
#define UDP_H

#include "net.h"
#include "ip.h"

// UDP Header structure
typedef struct __attribute__((packed)) {
    uint16_t src_port;    // Source port
    uint16_t dest_port;   // Destination port
    uint16_t length;      // Length of UDP header + data
    uint16_t checksum;    // Checksum
} udp_header_t;

// UDP Functions
void udp_init(void);
void udp_handle_packet(net_buffer_t* buffer, size_t offset, ip_header_t* ip_hdr);
int udp_send_packet(ip_addr_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, size_t length);
uint16_t udp_checksum(ip_header_t* ip_hdr, udp_header_t* udp_hdr, uint8_t* data, size_t length);

// Common UDP ports
#define UDP_PORT_DHCP_SERVER 67
#define UDP_PORT_DHCP_CLIENT 68
#define UDP_PORT_DNS         53
#define UDP_PORT_TFTP        69

#endif