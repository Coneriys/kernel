#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stddef.h>

// Network endianness conversion
#define htons(x) ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))
#define ntohs(x) htons(x)
#define htonl(x) ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8) | (((x) >> 24) & 0xFF))
#define ntohl(x) htonl(x)

// Network buffer management
#define NET_BUFFER_SIZE 1518  // MTU + Ethernet header
#define NET_MAX_BUFFERS 32

typedef struct {
    uint8_t data[NET_BUFFER_SIZE];
    size_t length;
    int in_use;
} net_buffer_t;

// MAC Address
typedef struct {
    uint8_t addr[6];
} mac_addr_t;

// IP Address  
typedef struct {
    uint8_t addr[4];
} ip_addr_t;

// Network interface
typedef struct {
    mac_addr_t mac;
    ip_addr_t ip;
    ip_addr_t netmask;
    ip_addr_t gateway;
    int active;
    char name[16];
} net_interface_t;

// Ethernet header
typedef struct __attribute__((packed)) {
    mac_addr_t dest;
    mac_addr_t src;
    uint16_t type;
} eth_header_t;

// Ethernet types
#define ETH_TYPE_IP   0x0800
#define ETH_TYPE_ARP  0x0806

// Function prototypes
void net_init(void);
net_buffer_t* net_alloc_buffer(void);
void net_free_buffer(net_buffer_t* buffer);
void net_send_packet(net_buffer_t* buffer);
void net_receive_packet(uint8_t* data, size_t length);

// Interface management
void net_set_interface(mac_addr_t mac, ip_addr_t ip, ip_addr_t netmask, ip_addr_t gateway);
net_interface_t* net_get_interface(void);
uint32_t get_local_ip(void);

// Utility functions
int mac_compare(mac_addr_t* a, mac_addr_t* b);
int ip_compare(ip_addr_t* a, ip_addr_t* b);
void mac_copy(mac_addr_t* dest, mac_addr_t* src);
void ip_copy(ip_addr_t* dest, ip_addr_t* src);
char* mac_to_string(mac_addr_t* mac);
char* ip_to_string(ip_addr_t* ip);

#endif