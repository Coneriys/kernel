#ifndef ARP_H
#define ARP_H

#include "net.h"

// ARP Header structure
typedef struct __attribute__((packed)) {
    uint16_t hardware_type;    // Hardware type (Ethernet = 1)
    uint16_t protocol_type;    // Protocol type (IP = 0x0800)
    uint8_t hardware_length;   // Hardware address length (6 for MAC)
    uint8_t protocol_length;   // Protocol address length (4 for IP)
    uint16_t operation;        // Operation (request = 1, reply = 2)
    mac_addr_t sender_mac;     // Sender MAC address
    ip_addr_t sender_ip;       // Sender IP address
    mac_addr_t target_mac;     // Target MAC address
    ip_addr_t target_ip;       // Target IP address
} arp_header_t;

// ARP Operations
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

// ARP Table entry
typedef struct {
    ip_addr_t ip;
    mac_addr_t mac;
    uint32_t timestamp;
    int valid;
} arp_entry_t;

#define ARP_TABLE_SIZE 32
#define ARP_TIMEOUT 300  // 5 minutes in seconds

// ARP Functions
void arp_init(void);
void arp_handle_packet(net_buffer_t* buffer, size_t offset);
int arp_resolve(ip_addr_t ip, mac_addr_t* mac);
void arp_send_request(ip_addr_t target_ip);
void arp_send_reply(ip_addr_t target_ip, mac_addr_t target_mac);
void arp_add_entry(ip_addr_t ip, mac_addr_t mac);
arp_entry_t* arp_lookup(ip_addr_t ip);

#endif