#ifndef DHCP_H
#define DHCP_H

#include "net.h"
#include "ip.h"

// DHCP Header structure
typedef struct __attribute__((packed)) {
    uint8_t op;          // Message op code / message type (1 = BOOTREQUEST, 2 = BOOTREPLY)
    uint8_t htype;       // Hardware address type (1 = Ethernet)
    uint8_t hlen;        // Hardware address length (6 for Ethernet)
    uint8_t hops;        // Client sets to zero
    uint32_t xid;        // Transaction ID
    uint16_t secs;       // Seconds elapsed since client began address acquisition
    uint16_t flags;      // Flags
    uint32_t ciaddr;     // Client IP address (only filled in if client is BOUND, RENEW or REBINDING)
    uint32_t yiaddr;     // 'your' (client) IP address
    uint32_t siaddr;     // IP address of next server to use in bootstrap
    uint32_t giaddr;     // Relay agent IP address
    uint8_t chaddr[16];  // Client hardware address
    uint8_t sname[64];   // Optional server host name
    uint8_t file[128];   // Boot file name
    uint32_t cookie;     // Magic cookie (0x63825363)
} dhcp_header_t;

// DHCP Message Types
#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTREPLY   2

// DHCP Options
#define DHCP_OPTION_PAD                    0
#define DHCP_OPTION_SUBNET_MASK           1
#define DHCP_OPTION_ROUTER                3
#define DHCP_OPTION_DNS_SERVER            6
#define DHCP_OPTION_HOSTNAME              12
#define DHCP_OPTION_REQUESTED_IP          50
#define DHCP_OPTION_LEASE_TIME            51
#define DHCP_OPTION_MESSAGE_TYPE          53
#define DHCP_OPTION_SERVER_IDENTIFIER     54
#define DHCP_OPTION_PARAMETER_REQUEST     55
#define DHCP_OPTION_CLIENT_IDENTIFIER     61
#define DHCP_OPTION_END                   255

// DHCP Message Types (Option 53)
#define DHCP_MSG_DISCOVER  1
#define DHCP_MSG_OFFER     2
#define DHCP_MSG_REQUEST   3
#define DHCP_MSG_DECLINE   4
#define DHCP_MSG_ACK       5
#define DHCP_MSG_NAK       6
#define DHCP_MSG_RELEASE   7
#define DHCP_MSG_INFORM    8

// DHCP Flags
#define DHCP_FLAG_BROADCAST 0x8000

// DHCP Magic Cookie
#define DHCP_MAGIC_COOKIE 0x63825363

// DHCP Client State
typedef enum {
    DHCP_STATE_INIT,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING
} dhcp_state_t;

typedef struct {
    dhcp_state_t state;
    uint32_t transaction_id;
    ip_addr_t offered_ip;
    ip_addr_t server_ip;
    ip_addr_t subnet_mask;
    ip_addr_t router;
    ip_addr_t dns_server;
    uint32_t lease_time;
    uint32_t renewal_time;
    uint32_t rebind_time;
    int active;
} dhcp_client_t;

// DHCP Functions
void dhcp_init(void);
void dhcp_start_discovery(void);
void dhcp_handle_packet(net_buffer_t* buffer, size_t offset);
void dhcp_send_discover(void);
void dhcp_send_request(ip_addr_t server_ip, ip_addr_t requested_ip);
int dhcp_parse_options(uint8_t* options, size_t length);
void dhcp_configure_interface(void);

// Helper functions
uint32_t dhcp_generate_xid(void);
void dhcp_add_option(uint8_t* buffer, size_t* offset, uint8_t type, uint8_t length, void* data);

#endif