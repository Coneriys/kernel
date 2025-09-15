#ifndef IP_H
#define IP_H

#include "net.h"

// IP Header structure
typedef struct __attribute__((packed)) {
    uint8_t version_ihl;      // Version (4 bits) + IHL (4 bits)
    uint8_t type_of_service;  // Type of service
    uint16_t total_length;    // Total length
    uint16_t identification;  // Identification
    uint16_t flags_fragment;  // Flags (3 bits) + Fragment offset (13 bits)
    uint8_t ttl;             // Time to live
    uint8_t protocol;        // Protocol
    uint16_t checksum;       // Header checksum
    ip_addr_t src_ip;        // Source IP
    ip_addr_t dest_ip;       // Destination IP
} ip_header_t;

// IP Protocol numbers
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP  6
#define IP_PROTOCOL_UDP  17

// IP Functions
void ip_init(void);
void ip_handle_packet(net_buffer_t* buffer, size_t offset);
int ip_send_packet(ip_addr_t dest, uint8_t protocol, uint8_t* data, size_t length);
uint16_t ip_checksum(void* data, size_t length);

#endif