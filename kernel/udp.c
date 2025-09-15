#include "../include/udp.h"
#include "../include/dhcp.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);

void udp_init(void) {
    terminal_writestring("UDP protocol initialized\n");
}

uint16_t udp_checksum(ip_header_t* ip_hdr, udp_header_t* udp_hdr, uint8_t* data, size_t length) {
    // UDP checksum calculation with pseudo-header
    uint32_t sum = 0;
    
    // Pseudo-header
    sum += (ip_hdr->src_ip.addr[0] << 8) + ip_hdr->src_ip.addr[1];
    sum += (ip_hdr->src_ip.addr[2] << 8) + ip_hdr->src_ip.addr[3];
    sum += (ip_hdr->dest_ip.addr[0] << 8) + ip_hdr->dest_ip.addr[1];
    sum += (ip_hdr->dest_ip.addr[2] << 8) + ip_hdr->dest_ip.addr[3];
    sum += IP_PROTOCOL_UDP;
    sum += ntohs(udp_hdr->length);
    
    // UDP header
    sum += ntohs(udp_hdr->src_port);
    sum += ntohs(udp_hdr->dest_port);
    sum += ntohs(udp_hdr->length);
    // Skip checksum field
    
    // Data
    uint16_t* data_ptr = (uint16_t*)data;
    size_t remaining = length;
    
    while (remaining > 1) {
        sum += ntohs(*data_ptr++);
        remaining -= 2;
    }
    
    if (remaining == 1) {
        sum += *(uint8_t*)data_ptr << 8;
    }
    
    // Add carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

void udp_handle_packet(net_buffer_t* buffer, size_t offset, ip_header_t* ip_hdr) {
    if (buffer->length < offset + sizeof(udp_header_t)) {
        return; // Packet too small
    }
    
    udp_header_t* udp_hdr = (udp_header_t*)(buffer->data + offset);
    
    uint16_t dest_port = ntohs(udp_hdr->dest_port);
    uint16_t src_port = ntohs(udp_hdr->src_port);
    uint16_t udp_length = ntohs(udp_hdr->length);
    
    if (udp_length < sizeof(udp_header_t)) {
        return; // Invalid length
    }
    
    size_t data_length = udp_length - sizeof(udp_header_t);
    uint8_t* udp_data = buffer->data + offset + sizeof(udp_header_t);
    
    // Verify checksum (optional for IPv4)
    if (udp_hdr->checksum != 0) {
        uint16_t orig_checksum = udp_hdr->checksum;
        udp_hdr->checksum = 0;
        uint16_t calculated_checksum = udp_checksum(ip_hdr, udp_hdr, udp_data, data_length);
        if (orig_checksum != calculated_checksum) {
            return; // Checksum mismatch
        }
        udp_hdr->checksum = orig_checksum;
    }
    
    // Handle different services
    switch (dest_port) {
        case UDP_PORT_DHCP_CLIENT:
            if (src_port == UDP_PORT_DHCP_SERVER) {
                dhcp_handle_packet(buffer, offset + sizeof(udp_header_t));
            }
            break;
        case UDP_PORT_DNS:
            // TODO: DNS handling
            break;
        default:
            // Unknown port
            break;
    }
}

int udp_send_packet(ip_addr_t dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, size_t length) {
    // Allocate buffer for UDP data
    size_t udp_packet_size = sizeof(udp_header_t) + length;
    uint8_t* udp_packet = (uint8_t*)kmalloc(udp_packet_size);
    if (!udp_packet) {
        return -1;
    }
    
    // Build UDP header
    udp_header_t* udp_hdr = (udp_header_t*)udp_packet;
    udp_hdr->src_port = htons(src_port);
    udp_hdr->dest_port = htons(dest_port);
    udp_hdr->length = htons(udp_packet_size);
    udp_hdr->checksum = 0; // We'll calculate this after building the IP header
    
    // Copy data
    uint8_t* udp_data = udp_packet + sizeof(udp_header_t);
    for (size_t i = 0; i < length; i++) {
        udp_data[i] = data[i];
    }
    
    // For checksum calculation, we need to build a pseudo IP header
    net_interface_t* iface = net_get_interface();
    if (!iface->active) {
        kfree(udp_packet);
        return -1;
    }
    
    // Create temporary IP header for checksum calculation
    ip_header_t temp_ip_hdr;
    ip_copy(&temp_ip_hdr.src_ip, &iface->ip);
    ip_copy(&temp_ip_hdr.dest_ip, &dest_ip);
    
    // Calculate UDP checksum
    udp_hdr->checksum = udp_checksum(&temp_ip_hdr, udp_hdr, udp_data, length);
    
    // Send via IP
    int result = ip_send_packet(dest_ip, IP_PROTOCOL_UDP, udp_packet, udp_packet_size);
    
    kfree(udp_packet);
    return result;
}