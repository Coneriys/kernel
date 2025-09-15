#include "../include/ip.h"
#include "../include/icmp.h"
#include "../include/arp.h"
#include "../include/udp.h"
#include "../include/tcp.h"

extern void terminal_writestring(const char* data);

static uint16_t ip_id_counter = 1;

void ip_init(void) {
    terminal_writestring("IP protocol initialized\n");
}

uint16_t ip_checksum(void* data, size_t length) {
    uint16_t* ptr = (uint16_t*)data;
    uint32_t sum = 0;
    
    // Sum all 16-bit words
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    // Add odd byte if present
    if (length == 1) {
        sum += *(uint8_t*)ptr << 8;
    }
    
    // Add carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

void ip_handle_packet(net_buffer_t* buffer, size_t offset) {
    if (buffer->length < offset + sizeof(ip_header_t)) {
        return; // Packet too small
    }
    
    ip_header_t* ip_hdr = (ip_header_t*)(buffer->data + offset);
    
    // Basic validation
    uint8_t version = (ip_hdr->version_ihl >> 4) & 0xF;
    if (version != 4) {
        return; // Not IPv4
    }
    
    uint8_t ihl = ip_hdr->version_ihl & 0xF;
    if (ihl < 5) {
        return; // Invalid header length
    }
    
    // Check if packet is for us
    net_interface_t* iface = net_get_interface();
    if (!ip_compare(&ip_hdr->dest_ip, &iface->ip)) {
        return; // Not for us
    }
    
    // Verify checksum
    uint16_t orig_checksum = ip_hdr->checksum;
    ip_hdr->checksum = 0;
    uint16_t calculated_checksum = ip_checksum(ip_hdr, ihl * 4);
    if (orig_checksum != calculated_checksum) {
        return; // Checksum mismatch
    }
    ip_hdr->checksum = orig_checksum;
    
    // Handle based on protocol
    size_t ip_header_size = ihl * 4;
    switch (ip_hdr->protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_handle_packet(buffer, offset + ip_header_size, ip_hdr);
            break;
        case IP_PROTOCOL_TCP:
            tcp_handle_packet(buffer->data + offset + ip_header_size, 
                             ntohs(ip_hdr->total_length) - ip_header_size,
                             (ip_hdr->src_ip.addr[0] << 24) | (ip_hdr->src_ip.addr[1] << 16) |
                             (ip_hdr->src_ip.addr[2] << 8) | ip_hdr->src_ip.addr[3],
                             (ip_hdr->dest_ip.addr[0] << 24) | (ip_hdr->dest_ip.addr[1] << 16) |
                             (ip_hdr->dest_ip.addr[2] << 8) | ip_hdr->dest_ip.addr[3]);
            break;
        case IP_PROTOCOL_UDP:
            udp_handle_packet(buffer, offset + ip_header_size, ip_hdr);
            break;
        default:
            // Unknown protocol
            break;
    }
}

int ip_send_packet(ip_addr_t dest, uint8_t protocol, uint8_t* data, size_t length) {
    net_interface_t* iface = net_get_interface();
    if (!iface->active) {
        return -1; // Interface not active
    }
    
    // Allocate buffer
    net_buffer_t* buffer = net_alloc_buffer();
    if (!buffer) {
        return -1; // No free buffers
    }
    
    // Resolve destination MAC address
    mac_addr_t dest_mac;
    if (!arp_resolve(dest, &dest_mac)) {
        net_free_buffer(buffer);
        return -1; // Could not resolve MAC
    }
    
    // Build Ethernet header
    eth_header_t* eth = (eth_header_t*)buffer->data;
    mac_copy(&eth->dest, &dest_mac);
    mac_copy(&eth->src, &iface->mac);
    eth->type = htons(ETH_TYPE_IP);
    
    // Build IP header
    ip_header_t* ip_hdr = (ip_header_t*)(buffer->data + sizeof(eth_header_t));
    ip_hdr->version_ihl = 0x45; // IPv4, 20 byte header
    ip_hdr->type_of_service = 0;
    ip_hdr->total_length = htons(sizeof(ip_header_t) + length);
    ip_hdr->identification = htons(ip_id_counter);
    ip_id_counter++;
    ip_hdr->flags_fragment = htons(0x4000); // Don't fragment
    ip_hdr->ttl = 64;
    ip_hdr->protocol = protocol;
    ip_hdr->checksum = 0;
    ip_copy(&ip_hdr->src_ip, &iface->ip);
    ip_copy(&ip_hdr->dest_ip, &dest);
    
    // Calculate checksum
    ip_hdr->checksum = ip_checksum(ip_hdr, sizeof(ip_header_t));
    
    // Copy payload
    uint8_t* payload = buffer->data + sizeof(eth_header_t) + sizeof(ip_header_t);
    for (size_t i = 0; i < length; i++) {
        payload[i] = data[i];
    }
    
    buffer->length = sizeof(eth_header_t) + sizeof(ip_header_t) + length;
    
    // Send packet
    net_send_packet(buffer);
    net_free_buffer(buffer);
    
    return 0;
}