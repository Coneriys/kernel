#include "../include/dhcp.h"
#include "../include/udp.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);

static dhcp_client_t dhcp_client;
static uint32_t dhcp_xid_counter = 1;

void dhcp_init(void) {
    dhcp_client.state = DHCP_STATE_INIT;
    dhcp_client.active = 0;
    dhcp_client.transaction_id = 0;
    
    terminal_writestring("DHCP client initialized\n");
}

uint32_t dhcp_generate_xid(void) {
    return dhcp_xid_counter++;
}

void dhcp_add_option(uint8_t* buffer, size_t* offset, uint8_t type, uint8_t length, void* data) {
    buffer[*offset] = type;
    (*offset)++;
    buffer[*offset] = length;
    (*offset)++;
    
    if (data && length > 0) {
        uint8_t* data_bytes = (uint8_t*)data;
        for (uint8_t i = 0; i < length; i++) {
            buffer[*offset] = data_bytes[i];
            (*offset)++;
        }
    }
}

void dhcp_start_discovery(void) {
    if (dhcp_client.active) {
        return; // Already running
    }
    
    terminal_writestring("Starting DHCP discovery...\n");
    dhcp_client.state = DHCP_STATE_INIT;
    dhcp_client.active = 1;
    dhcp_send_discover();
}

void dhcp_send_discover(void) {
    net_interface_t* iface = net_get_interface();
    if (!iface) {
        return;
    }
    
    // Allocate buffer for DHCP packet
    size_t dhcp_size = sizeof(dhcp_header_t) + 312; // 312 bytes for options
    uint8_t* dhcp_packet = (uint8_t*)kmalloc(dhcp_size);
    if (!dhcp_packet) {
        return;
    }
    
    // Clear packet
    for (size_t i = 0; i < dhcp_size; i++) {
        dhcp_packet[i] = 0;
    }
    
    // Build DHCP header
    dhcp_header_t* dhcp_hdr = (dhcp_header_t*)dhcp_packet;
    dhcp_hdr->op = DHCP_BOOTREQUEST;
    dhcp_hdr->htype = 1; // Ethernet
    dhcp_hdr->hlen = 6;  // MAC address length
    dhcp_hdr->hops = 0;
    dhcp_hdr->xid = htonl(dhcp_generate_xid());
    dhcp_client.transaction_id = ntohl(dhcp_hdr->xid);
    dhcp_hdr->secs = 0;
    dhcp_hdr->flags = htons(DHCP_FLAG_BROADCAST);
    dhcp_hdr->ciaddr = 0;
    dhcp_hdr->yiaddr = 0;
    dhcp_hdr->siaddr = 0;
    dhcp_hdr->giaddr = 0;
    
    // Copy MAC address
    for (int i = 0; i < 6; i++) {
        dhcp_hdr->chaddr[i] = iface->mac.addr[i];
    }
    
    dhcp_hdr->cookie = htonl(DHCP_MAGIC_COOKIE);
    
    // Add options
    uint8_t* options = dhcp_packet + sizeof(dhcp_header_t);
    size_t options_offset = 0;
    
    // Message Type (DISCOVER)
    uint8_t msg_type = DHCP_MSG_DISCOVER;
    dhcp_add_option(options, &options_offset, DHCP_OPTION_MESSAGE_TYPE, 1, &msg_type);
    
    // Client Identifier (MAC address)
    uint8_t client_id[7];
    client_id[0] = 1; // Hardware type (Ethernet)
    for (int i = 0; i < 6; i++) {
        client_id[i + 1] = iface->mac.addr[i];
    }
    dhcp_add_option(options, &options_offset, DHCP_OPTION_CLIENT_IDENTIFIER, 7, client_id);
    
    // Parameter Request List
    uint8_t param_list[] = {
        DHCP_OPTION_SUBNET_MASK,
        DHCP_OPTION_ROUTER,
        DHCP_OPTION_DNS_SERVER,
        DHCP_OPTION_LEASE_TIME
    };
    dhcp_add_option(options, &options_offset, DHCP_OPTION_PARAMETER_REQUEST, 4, param_list);
    
    // End option
    dhcp_add_option(options, &options_offset, DHCP_OPTION_END, 0, NULL);
    
    // Send DHCP DISCOVER
    ip_addr_t broadcast_ip = {{255, 255, 255, 255}};
    udp_send_packet(broadcast_ip, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER, 
                   dhcp_packet, sizeof(dhcp_header_t) + options_offset);
    
    dhcp_client.state = DHCP_STATE_SELECTING;
    
    kfree(dhcp_packet);
    terminal_writestring("DHCP DISCOVER sent\n");
}

void dhcp_send_request(ip_addr_t server_ip, ip_addr_t requested_ip) {
    net_interface_t* iface = net_get_interface();
    if (!iface) {
        return;
    }
    
    // Allocate buffer for DHCP packet
    size_t dhcp_size = sizeof(dhcp_header_t) + 312;
    uint8_t* dhcp_packet = (uint8_t*)kmalloc(dhcp_size);
    if (!dhcp_packet) {
        return;
    }
    
    // Clear packet
    for (size_t i = 0; i < dhcp_size; i++) {
        dhcp_packet[i] = 0;
    }
    
    // Build DHCP header
    dhcp_header_t* dhcp_hdr = (dhcp_header_t*)dhcp_packet;
    dhcp_hdr->op = DHCP_BOOTREQUEST;
    dhcp_hdr->htype = 1;
    dhcp_hdr->hlen = 6;
    dhcp_hdr->hops = 0;
    dhcp_hdr->xid = htonl(dhcp_client.transaction_id);
    dhcp_hdr->secs = 0;
    dhcp_hdr->flags = htons(DHCP_FLAG_BROADCAST);
    dhcp_hdr->ciaddr = 0;
    dhcp_hdr->yiaddr = 0;
    dhcp_hdr->siaddr = 0;
    dhcp_hdr->giaddr = 0;
    
    // Copy MAC address
    for (int i = 0; i < 6; i++) {
        dhcp_hdr->chaddr[i] = iface->mac.addr[i];
    }
    
    dhcp_hdr->cookie = htonl(DHCP_MAGIC_COOKIE);
    
    // Add options
    uint8_t* options = dhcp_packet + sizeof(dhcp_header_t);
    size_t options_offset = 0;
    
    // Message Type (REQUEST)
    uint8_t msg_type = DHCP_MSG_REQUEST;
    dhcp_add_option(options, &options_offset, DHCP_OPTION_MESSAGE_TYPE, 1, &msg_type);
    
    // Requested IP Address
    dhcp_add_option(options, &options_offset, DHCP_OPTION_REQUESTED_IP, 4, &requested_ip);
    
    // Server Identifier
    dhcp_add_option(options, &options_offset, DHCP_OPTION_SERVER_IDENTIFIER, 4, &server_ip);
    
    // Client Identifier
    uint8_t client_id[7];
    client_id[0] = 1;
    for (int i = 0; i < 6; i++) {
        client_id[i + 1] = iface->mac.addr[i];
    }
    dhcp_add_option(options, &options_offset, DHCP_OPTION_CLIENT_IDENTIFIER, 7, client_id);
    
    // End option
    dhcp_add_option(options, &options_offset, DHCP_OPTION_END, 0, NULL);
    
    // Send DHCP REQUEST
    ip_addr_t broadcast_ip = {{255, 255, 255, 255}};
    udp_send_packet(broadcast_ip, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER,
                   dhcp_packet, sizeof(dhcp_header_t) + options_offset);
    
    dhcp_client.state = DHCP_STATE_REQUESTING;
    
    kfree(dhcp_packet);
    terminal_writestring("DHCP REQUEST sent\n");
}

int dhcp_parse_options(uint8_t* options, size_t length) {
    size_t offset = 0;
    uint8_t msg_type = 0;
    
    while (offset < length) {
        uint8_t option_type = options[offset++];
        
        if (option_type == DHCP_OPTION_PAD) {
            continue;
        }
        
        if (option_type == DHCP_OPTION_END) {
            break;
        }
        
        if (offset >= length) {
            break;
        }
        
        uint8_t option_length = options[offset++];
        
        if (offset + option_length > length) {
            break;
        }
        
        switch (option_type) {
            case DHCP_OPTION_MESSAGE_TYPE:
                if (option_length == 1) {
                    msg_type = options[offset];
                }
                break;
                
            case DHCP_OPTION_SUBNET_MASK:
                if (option_length == 4) {
                    for (int i = 0; i < 4; i++) {
                        dhcp_client.subnet_mask.addr[i] = options[offset + i];
                    }
                }
                break;
                
            case DHCP_OPTION_ROUTER:
                if (option_length >= 4) {
                    for (int i = 0; i < 4; i++) {
                        dhcp_client.router.addr[i] = options[offset + i];
                    }
                }
                break;
                
            case DHCP_OPTION_DNS_SERVER:
                if (option_length >= 4) {
                    for (int i = 0; i < 4; i++) {
                        dhcp_client.dns_server.addr[i] = options[offset + i];
                    }
                }
                break;
                
            case DHCP_OPTION_LEASE_TIME:
                if (option_length == 4) {
                    dhcp_client.lease_time = ntohl(*(uint32_t*)(options + offset));
                }
                break;
                
            case DHCP_OPTION_SERVER_IDENTIFIER:
                if (option_length == 4) {
                    for (int i = 0; i < 4; i++) {
                        dhcp_client.server_ip.addr[i] = options[offset + i];
                    }
                }
                break;
        }
        
        offset += option_length;
    }
    
    return msg_type;
}

void dhcp_configure_interface(void) {
    net_set_interface(net_get_interface()->mac, dhcp_client.offered_ip, 
                     dhcp_client.subnet_mask, dhcp_client.router);
    
    terminal_writestring("DHCP configuration applied:\n");
    terminal_writestring("  IP: ");
    terminal_writestring(ip_to_string(&dhcp_client.offered_ip));
    terminal_writestring("\n  Netmask: ");
    terminal_writestring(ip_to_string(&dhcp_client.subnet_mask));
    terminal_writestring("\n  Gateway: ");
    terminal_writestring(ip_to_string(&dhcp_client.router));
    terminal_writestring("\n");
}

void dhcp_handle_packet(net_buffer_t* buffer, size_t offset) {
    if (buffer->length < offset + sizeof(dhcp_header_t)) {
        return;
    }
    
    dhcp_header_t* dhcp_hdr = (dhcp_header_t*)(buffer->data + offset);
    
    // Check if this is a reply and for us
    if (dhcp_hdr->op != DHCP_BOOTREPLY || 
        ntohl(dhcp_hdr->xid) != dhcp_client.transaction_id) {
        return;
    }
    
    // Check magic cookie
    if (ntohl(dhcp_hdr->cookie) != DHCP_MAGIC_COOKIE) {
        return;
    }
    
    // Parse options
    uint8_t* options = buffer->data + offset + sizeof(dhcp_header_t);
    size_t options_length = buffer->length - offset - sizeof(dhcp_header_t);
    
    uint8_t msg_type = dhcp_parse_options(options, options_length);
    
    switch (dhcp_client.state) {
        case DHCP_STATE_SELECTING:
            if (msg_type == DHCP_MSG_OFFER) {
                terminal_writestring("Received DHCP OFFER\n");
                
                // Store offered IP
                for (int i = 0; i < 4; i++) {
                    dhcp_client.offered_ip.addr[i] = (dhcp_hdr->yiaddr >> (i * 8)) & 0xFF;
                }
                
                // Send REQUEST
                dhcp_send_request(dhcp_client.server_ip, dhcp_client.offered_ip);
            }
            break;
            
        case DHCP_STATE_REQUESTING:
            if (msg_type == DHCP_MSG_ACK) {
                terminal_writestring("Received DHCP ACK\n");
                dhcp_client.state = DHCP_STATE_BOUND;
                dhcp_configure_interface();
            } else if (msg_type == DHCP_MSG_NAK) {
                terminal_writestring("Received DHCP NAK, restarting...\n");
                dhcp_client.state = DHCP_STATE_INIT;
                dhcp_send_discover();
            }
            break;
            
        default:
            break;
    }
}