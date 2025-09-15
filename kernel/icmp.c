#include "../include/icmp.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);

void icmp_init(void) {
    terminal_writestring("ICMP protocol initialized\n");
}

void icmp_handle_packet(net_buffer_t* buffer, size_t offset, ip_header_t* ip_hdr) {
    if (buffer->length < offset + sizeof(icmp_header_t)) {
        return; // Packet too small
    }
    
    icmp_header_t* icmp_hdr = (icmp_header_t*)(buffer->data + offset);
    
    // Calculate payload size
    uint16_t ip_total_length = ntohs(ip_hdr->total_length);
    uint8_t ip_header_length = (ip_hdr->version_ihl & 0xF) * 4;
    size_t icmp_length = ip_total_length - ip_header_length;
    size_t payload_length = icmp_length - sizeof(icmp_header_t);
    
    switch (icmp_hdr->type) {
        case ICMP_TYPE_ECHO_REQUEST:
            // Respond to ping
            terminal_writestring("Received ICMP ping request\n");
            icmp_send_echo_reply(ip_hdr->src_ip, icmp_hdr->id, icmp_hdr->sequence,
                               buffer->data + offset + sizeof(icmp_header_t), payload_length);
            break;
        case ICMP_TYPE_ECHO_REPLY:
            terminal_writestring("Received ICMP ping reply\n");
            break;
        default:
            // Unknown ICMP type
            break;
    }
}

void icmp_send_echo_reply(ip_addr_t dest, uint16_t id, uint16_t sequence, uint8_t* data, size_t length) {
    // Allocate buffer for ICMP data
    size_t icmp_data_size = sizeof(icmp_header_t) + length;
    uint8_t* icmp_data = (uint8_t*)kmalloc(icmp_data_size);
    if (!icmp_data) {
        return;
    }
    
    // Build ICMP header
    icmp_header_t* icmp_hdr = (icmp_header_t*)icmp_data;
    icmp_hdr->type = ICMP_TYPE_ECHO_REPLY;
    icmp_hdr->code = 0;
    icmp_hdr->checksum = 0;
    icmp_hdr->id = id;
    icmp_hdr->sequence = sequence;
    
    // Copy payload
    uint8_t* payload = icmp_data + sizeof(icmp_header_t);
    for (size_t i = 0; i < length; i++) {
        payload[i] = data[i];
    }
    
    // Calculate checksum
    icmp_hdr->checksum = ip_checksum(icmp_data, icmp_data_size);
    
    // Send via IP
    ip_send_packet(dest, IP_PROTOCOL_ICMP, icmp_data, icmp_data_size);
    
    kfree(icmp_data);
}

int icmp_send_ping(ip_addr_t dest, uint16_t id, uint16_t sequence, uint8_t* data, size_t length) {
    // Allocate buffer for ICMP data
    size_t icmp_data_size = sizeof(icmp_header_t) + length;
    uint8_t* icmp_data = (uint8_t*)kmalloc(icmp_data_size);
    if (!icmp_data) {
        return -1;
    }
    
    // Build ICMP header
    icmp_header_t* icmp_hdr = (icmp_header_t*)icmp_data;
    icmp_hdr->type = ICMP_TYPE_ECHO_REQUEST;
    icmp_hdr->code = 0;
    icmp_hdr->checksum = 0;
    icmp_hdr->id = id;
    icmp_hdr->sequence = sequence;
    
    // Copy payload
    uint8_t* payload = icmp_data + sizeof(icmp_header_t);
    for (size_t i = 0; i < length; i++) {
        payload[i] = data[i];
    }
    
    // Calculate checksum
    icmp_hdr->checksum = ip_checksum(icmp_data, icmp_data_size);
    
    // Send via IP
    int result = ip_send_packet(dest, IP_PROTOCOL_ICMP, icmp_data, icmp_data_size);
    
    kfree(icmp_data);
    return result;
}