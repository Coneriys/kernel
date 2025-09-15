#include "../include/arp.h"

extern void terminal_writestring(const char* data);

static arp_entry_t arp_table[ARP_TABLE_SIZE];

void arp_init(void) {
    // Initialize ARP table
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        arp_table[i].valid = 0;
        arp_table[i].timestamp = 0;
    }
    
    terminal_writestring("ARP protocol initialized\n");
}

void arp_handle_packet(net_buffer_t* buffer, size_t offset) {
    if (buffer->length < offset + sizeof(arp_header_t)) {
        return; // Packet too small
    }
    
    arp_header_t* arp_hdr = (arp_header_t*)(buffer->data + offset);
    
    // Validate ARP packet
    if (ntohs(arp_hdr->hardware_type) != 1 ||  // Ethernet
        ntohs(arp_hdr->protocol_type) != 0x0800 || // IP
        arp_hdr->hardware_length != 6 ||
        arp_hdr->protocol_length != 4) {
        return; // Invalid ARP packet
    }
    
    net_interface_t* iface = net_get_interface();
    if (!iface->active) {
        return;
    }
    
    // Check if the packet is for us
    if (!ip_compare(&arp_hdr->target_ip, &iface->ip)) {
        return; // Not for us
    }
    
    // Add sender to ARP table
    arp_add_entry(arp_hdr->sender_ip, arp_hdr->sender_mac);
    
    uint16_t operation = ntohs(arp_hdr->operation);
    
    if (operation == ARP_OP_REQUEST) {
        // Send ARP reply
        arp_send_reply(arp_hdr->sender_ip, arp_hdr->sender_mac);
    }
    // ARP replies are handled by adding to table above
}

int arp_resolve(ip_addr_t ip, mac_addr_t* mac) {
    // Check ARP table first
    arp_entry_t* entry = arp_lookup(ip);
    if (entry && entry->valid) {
        mac_copy(mac, &entry->mac);
        return 1;
    }
    
    // Send ARP request
    arp_send_request(ip);
    
    // For now, return failure (in real implementation, would wait for reply)
    return 0;
}

void arp_send_request(ip_addr_t target_ip) {
    net_interface_t* iface = net_get_interface();
    if (!iface->active) {
        return;
    }
    
    net_buffer_t* buffer = net_alloc_buffer();
    if (!buffer) {
        return;
    }
    
    // Build Ethernet header (broadcast)
    eth_header_t* eth = (eth_header_t*)buffer->data;
    for (int i = 0; i < 6; i++) {
        eth->dest.addr[i] = 0xFF; // Broadcast MAC
    }
    mac_copy(&eth->src, &iface->mac);
    eth->type = htons(ETH_TYPE_ARP);
    
    // Build ARP header
    arp_header_t* arp_hdr = (arp_header_t*)(buffer->data + sizeof(eth_header_t));
    arp_hdr->hardware_type = htons(1); // Ethernet
    arp_hdr->protocol_type = htons(0x0800); // IP
    arp_hdr->hardware_length = 6;
    arp_hdr->protocol_length = 4;
    arp_hdr->operation = htons(ARP_OP_REQUEST);
    mac_copy(&arp_hdr->sender_mac, &iface->mac);
    ip_copy(&arp_hdr->sender_ip, &iface->ip);
    // Target MAC is unknown (all zeros)
    for (int i = 0; i < 6; i++) {
        arp_hdr->target_mac.addr[i] = 0x00;
    }
    ip_copy(&arp_hdr->target_ip, &target_ip);
    
    buffer->length = sizeof(eth_header_t) + sizeof(arp_header_t);
    
    net_send_packet(buffer);
    net_free_buffer(buffer);
}

void arp_send_reply(ip_addr_t target_ip, mac_addr_t target_mac) {
    net_interface_t* iface = net_get_interface();
    if (!iface->active) {
        return;
    }
    
    net_buffer_t* buffer = net_alloc_buffer();
    if (!buffer) {
        return;
    }
    
    // Build Ethernet header
    eth_header_t* eth = (eth_header_t*)buffer->data;
    mac_copy(&eth->dest, &target_mac);
    mac_copy(&eth->src, &iface->mac);
    eth->type = htons(ETH_TYPE_ARP);
    
    // Build ARP header
    arp_header_t* arp_hdr = (arp_header_t*)(buffer->data + sizeof(eth_header_t));
    arp_hdr->hardware_type = htons(1); // Ethernet
    arp_hdr->protocol_type = htons(0x0800); // IP
    arp_hdr->hardware_length = 6;
    arp_hdr->protocol_length = 4;
    arp_hdr->operation = htons(ARP_OP_REPLY);
    mac_copy(&arp_hdr->sender_mac, &iface->mac);
    ip_copy(&arp_hdr->sender_ip, &iface->ip);
    mac_copy(&arp_hdr->target_mac, &target_mac);
    ip_copy(&arp_hdr->target_ip, &target_ip);
    
    buffer->length = sizeof(eth_header_t) + sizeof(arp_header_t);
    
    net_send_packet(buffer);
    net_free_buffer(buffer);
}

void arp_add_entry(ip_addr_t ip, mac_addr_t mac) {
    // Find existing entry or empty slot
    int slot = -1;
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && ip_compare(&arp_table[i].ip, &ip)) {
            slot = i;
            break;
        } else if (!arp_table[i].valid && slot == -1) {
            slot = i;
        }
    }
    
    if (slot == -1) {
        // Table full, find oldest entry
        uint32_t oldest_time = arp_table[0].timestamp;
        slot = 0;
        for (int i = 1; i < ARP_TABLE_SIZE; i++) {
            if (arp_table[i].timestamp < oldest_time) {
                oldest_time = arp_table[i].timestamp;
                slot = i;
            }
        }
    }
    
    // Add/update entry
    ip_copy(&arp_table[slot].ip, &ip);
    mac_copy(&arp_table[slot].mac, &mac);
    arp_table[slot].timestamp = 0; // TODO: Get current time
    arp_table[slot].valid = 1;
}

arp_entry_t* arp_lookup(ip_addr_t ip) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && ip_compare(&arp_table[i].ip, &ip)) {
            return &arp_table[i];
        }
    }
    return NULL;
}