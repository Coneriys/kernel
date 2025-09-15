#ifndef ICMP_H
#define ICMP_H

#include "net.h"
#include "ip.h"

// ICMP Header structure
typedef struct __attribute__((packed)) {
    uint8_t type;        // ICMP type
    uint8_t code;        // ICMP code
    uint16_t checksum;   // Checksum
    uint16_t id;         // Identifier
    uint16_t sequence;   // Sequence number
} icmp_header_t;

// ICMP Types
#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

// ICMP Functions
void icmp_init(void);
void icmp_handle_packet(net_buffer_t* buffer, size_t offset, ip_header_t* ip_hdr);
void icmp_send_echo_reply(ip_addr_t dest, uint16_t id, uint16_t sequence, uint8_t* data, size_t length);
int icmp_send_ping(ip_addr_t dest, uint16_t id, uint16_t sequence, uint8_t* data, size_t length);

#endif