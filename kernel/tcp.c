#include "../include/tcp.h"
#include "../include/net.h"
#include "../include/ip.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);
extern size_t strlen(const char* str);

// Global variables
static tcp_connection_t* tcp_connections = NULL;
static tcp_socket_t tcp_sockets[64];
static uint16_t next_port = 49152; // Start of dynamic port range
static int tcp_initialized = 0;

// Helper function to convert number to string
static void tcp_itoa(uint32_t value, char* str) {
    int pos = 0;
    
    if (value == 0) {
        str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        
        while (value > 0) {
            temp[temp_pos++] = '0' + (value % 10);
            value /= 10;
        }
        
        while (temp_pos > 0) {
            str[pos++] = temp[--temp_pos];
        }
    }
    
    str[pos] = '\0';
}

void tcp_init(void) {
    terminal_writestring("Initializing TCP protocol...\n");
    
    // Initialize socket table
    for (int i = 0; i < 64; i++) {
        tcp_sockets[i].connection = NULL;
        tcp_sockets[i].socket_id = -1;
        tcp_sockets[i].is_listening = 0;
    }
    
    tcp_connections = NULL;
    tcp_initialized = 1;
    
    terminal_writestring("TCP protocol initialized\n");
}

uint16_t tcp_checksum(tcp_header_t* tcp_hdr, uint8_t* data, size_t data_len, uint32_t src_ip, uint32_t dst_ip) {
    uint32_t sum = 0;
    
    // Create pseudo header
    tcp_pseudo_header_t pseudo;
    pseudo.src_ip = src_ip;
    pseudo.dst_ip = dst_ip;
    pseudo.zero = 0;
    pseudo.protocol = 6; // TCP protocol number
    pseudo.tcp_length = htons(sizeof(tcp_header_t) + data_len);
    
    // Sum pseudo header
    uint16_t* ptr = (uint16_t*)&pseudo;
    for (size_t i = 0; i < sizeof(tcp_pseudo_header_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }
    
    // Sum TCP header
    uint16_t old_checksum = tcp_hdr->checksum;
    tcp_hdr->checksum = 0;
    
    ptr = (uint16_t*)tcp_hdr;
    for (size_t i = 0; i < sizeof(tcp_header_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }
    
    tcp_hdr->checksum = old_checksum;
    
    // Sum data
    ptr = (uint16_t*)data;
    for (size_t i = 0; i < data_len / 2; i++) {
        sum += ntohs(ptr[i]);
    }
    
    // Add left-over byte if any
    if (data_len % 2) {
        sum += ((uint16_t)data[data_len - 1]) << 8;
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return htons(~sum);
}

tcp_connection_t* tcp_create_connection(void) {
    tcp_connection_t* conn = (tcp_connection_t*)kmalloc(sizeof(tcp_connection_t));
    if (!conn) return NULL;
    
    // Initialize connection
    conn->state = TCP_CLOSED;
    conn->send_seq = 1000; // Initial sequence number
    conn->send_ack = 0;
    conn->recv_seq = 0;
    conn->recv_ack = 0;
    conn->send_window = 8192;
    conn->recv_window = 8192;
    
    // Allocate buffers
    conn->send_buffer_size = 4096;
    conn->recv_buffer_size = 4096;
    conn->send_buffer = (uint8_t*)kmalloc(conn->send_buffer_size);
    conn->recv_buffer = (uint8_t*)kmalloc(conn->recv_buffer_size);
    conn->send_buffer_used = 0;
    conn->recv_buffer_used = 0;
    
    // Add to connection list
    conn->next = tcp_connections;
    tcp_connections = conn;
    
    return conn;
}

void tcp_destroy_connection(tcp_connection_t* conn) {
    if (!conn) return;
    
    // Remove from connection list
    if (tcp_connections == conn) {
        tcp_connections = conn->next;
    } else {
        tcp_connection_t* prev = tcp_connections;
        while (prev && prev->next != conn) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = conn->next;
        }
    }
    
    // Free buffers
    if (conn->send_buffer) kfree(conn->send_buffer);
    if (conn->recv_buffer) kfree(conn->recv_buffer);
    
    kfree(conn);
}

tcp_connection_t* tcp_find_connection(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port) {
    tcp_connection_t* conn = tcp_connections;
    
    while (conn) {
        if (conn->local_port == local_port &&
            conn->remote_port == remote_port &&
            conn->local_ip == local_ip &&
            conn->remote_ip == remote_ip) {
            return conn;
        }
        conn = conn->next;
    }
    
    return NULL;
}

uint16_t tcp_allocate_port(void) {
    uint16_t port = next_port++;
    if (next_port > 65535) {
        next_port = 49152;
    }
    return port;
}

int tcp_is_port_available(uint16_t port) {
    tcp_connection_t* conn = tcp_connections;
    
    while (conn) {
        if (conn->local_port == port) {
            return 0;
        }
        conn = conn->next;
    }
    
    return 1;
}

void tcp_send_packet(tcp_connection_t* conn, uint8_t flags, uint8_t* data, size_t data_len) {
    // Calculate total packet size
    size_t total_size = sizeof(ip_header_t) + sizeof(tcp_header_t) + data_len;
    uint8_t* packet = (uint8_t*)kmalloc(total_size);
    if (!packet) return;
    
    // Prepare TCP header
    tcp_header_t* tcp_hdr = (tcp_header_t*)(packet + sizeof(ip_header_t));
    tcp_hdr->src_port = htons(conn->local_port);
    tcp_hdr->dst_port = htons(conn->remote_port);
    tcp_hdr->seq_num = htonl(conn->send_seq);
    tcp_hdr->ack_num = htonl(conn->send_ack);
    tcp_hdr->data_offset = (sizeof(tcp_header_t) / 4) << 4; // No options
    tcp_hdr->flags = flags;
    tcp_hdr->window = htons(conn->recv_window);
    tcp_hdr->checksum = 0;
    tcp_hdr->urgent_ptr = 0;
    
    // Copy data if any
    if (data && data_len > 0) {
        uint8_t* tcp_data = packet + sizeof(ip_header_t) + sizeof(tcp_header_t);
        for (size_t i = 0; i < data_len; i++) {
            tcp_data[i] = data[i];
        }
    }
    
    // Calculate checksum
    tcp_hdr->checksum = tcp_checksum(tcp_hdr, 
                                     packet + sizeof(ip_header_t) + sizeof(tcp_header_t),
                                     data_len, conn->local_ip, conn->remote_ip);
    
    // Send via IP layer
    ip_addr_t dest_ip;
    dest_ip.addr[0] = (conn->remote_ip >> 24) & 0xFF;
    dest_ip.addr[1] = (conn->remote_ip >> 16) & 0xFF;
    dest_ip.addr[2] = (conn->remote_ip >> 8) & 0xFF;
    dest_ip.addr[3] = conn->remote_ip & 0xFF;
    
    ip_send_packet(dest_ip, 6, packet + sizeof(ip_header_t), 
                   sizeof(tcp_header_t) + data_len);
    
    // Update sequence number for data and SYN/FIN
    if (data_len > 0 || (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN))) {
        conn->send_seq += data_len;
        if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)) {
            conn->send_seq++;
        }
    }
    
    kfree(packet);
}

void tcp_send_syn(tcp_connection_t* conn) {
    conn->state = TCP_SYN_SENT;
    tcp_send_packet(conn, TCP_FLAG_SYN, NULL, 0);
}

void tcp_send_syn_ack(tcp_connection_t* conn) {
    conn->state = TCP_SYN_RECEIVED;
    tcp_send_packet(conn, TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
}

void tcp_send_ack(tcp_connection_t* conn) {
    tcp_send_packet(conn, TCP_FLAG_ACK, NULL, 0);
}

void tcp_send_fin(tcp_connection_t* conn) {
    tcp_send_packet(conn, TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
}

void tcp_send_rst(tcp_connection_t* conn) {
    tcp_send_packet(conn, TCP_FLAG_RST, NULL, 0);
}

void tcp_state_machine(tcp_connection_t* conn, tcp_header_t* tcp_hdr, uint8_t* data, size_t data_len) {
    uint8_t flags = tcp_hdr->flags;
    uint32_t seq = ntohl(tcp_hdr->seq_num);
    uint32_t ack = ntohl(tcp_hdr->ack_num);
    
    switch (conn->state) {
        case TCP_CLOSED:
            if (flags & TCP_FLAG_SYN) {
                // Passive open
                conn->recv_seq = seq;
                conn->send_ack = seq + 1;
                tcp_send_syn_ack(conn);
            }
            break;
            
        case TCP_LISTEN:
            if (flags & TCP_FLAG_SYN) {
                conn->recv_seq = seq;
                conn->send_ack = seq + 1;
                tcp_send_syn_ack(conn);
            }
            break;
            
        case TCP_SYN_SENT:
            if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
                // SYN-ACK received
                conn->recv_seq = seq;
                conn->send_ack = seq + 1;
                conn->state = TCP_ESTABLISHED;
                tcp_send_ack(conn);
                
                terminal_writestring("TCP connection established!\n");
            }
            break;
            
        case TCP_SYN_RECEIVED:
            if (flags & TCP_FLAG_ACK) {
                conn->state = TCP_ESTABLISHED;
                terminal_writestring("TCP connection established!\n");
            }
            break;
            
        case TCP_ESTABLISHED:
            if (flags & TCP_FLAG_FIN) {
                // Close request
                conn->send_ack = seq + 1;
                tcp_send_ack(conn);
                conn->state = TCP_CLOSE_WAIT;
                
                // Send our FIN
                tcp_send_fin(conn);
                conn->state = TCP_LAST_ACK;
            } else if (data_len > 0) {
                // Data received
                if (conn->recv_buffer_used + data_len <= conn->recv_buffer_size) {
                    // Copy data to receive buffer
                    for (size_t i = 0; i < data_len; i++) {
                        conn->recv_buffer[conn->recv_buffer_used + i] = data[i];
                    }
                    conn->recv_buffer_used += data_len;
                    
                    // Update acknowledgment
                    conn->send_ack = seq + data_len;
                    tcp_send_ack(conn);
                }
            }
            break;
            
        case TCP_FIN_WAIT_1:
            if ((flags & (TCP_FLAG_FIN | TCP_FLAG_ACK)) == (TCP_FLAG_FIN | TCP_FLAG_ACK)) {
                conn->send_ack = seq + 1;
                tcp_send_ack(conn);
                conn->state = TCP_TIME_WAIT;
            } else if (flags & TCP_FLAG_ACK) {
                conn->state = TCP_FIN_WAIT_2;
            }
            break;
            
        case TCP_FIN_WAIT_2:
            if (flags & TCP_FLAG_FIN) {
                conn->send_ack = seq + 1;
                tcp_send_ack(conn);
                conn->state = TCP_TIME_WAIT;
            }
            break;
            
        case TCP_LAST_ACK:
            if (flags & TCP_FLAG_ACK) {
                conn->state = TCP_CLOSED;
                terminal_writestring("TCP connection closed\n");
            }
            break;
            
        case TCP_TIME_WAIT:
            // Wait for 2MSL timeout
            conn->state = TCP_CLOSED;
            break;
            
        default:
            break;
    }
}

void tcp_handle_packet(uint8_t* packet, size_t len, uint32_t src_ip, uint32_t dst_ip) {
    if (len < sizeof(tcp_header_t)) return;
    
    tcp_header_t* tcp_hdr = (tcp_header_t*)packet;
    uint16_t src_port = ntohs(tcp_hdr->src_port);
    uint16_t dst_port = ntohs(tcp_hdr->dst_port);
    
    // Calculate data offset and length
    uint8_t header_len = (tcp_hdr->data_offset >> 4) * 4;
    if (header_len < sizeof(tcp_header_t) || header_len > len) return;
    
    size_t data_len = len - header_len;
    uint8_t* data = packet + header_len;
    
    // Verify checksum
    uint16_t received_checksum = tcp_hdr->checksum;
    uint16_t calculated_checksum = tcp_checksum(tcp_hdr, data, data_len, src_ip, dst_ip);
    
    if (received_checksum != calculated_checksum) {
        terminal_writestring("TCP checksum error\n");
        return;
    }
    
    // Find existing connection
    tcp_connection_t* conn = tcp_find_connection(dst_ip, dst_port, src_ip, src_port);
    
    if (!conn) {
        // Check for listening socket
        for (int i = 0; i < 64; i++) {
            if (tcp_sockets[i].is_listening && 
                tcp_sockets[i].connection &&
                tcp_sockets[i].connection->local_port == dst_port) {
                
                // Create new connection for incoming SYN
                if (tcp_hdr->flags & TCP_FLAG_SYN) {
                    conn = tcp_create_connection();
                    if (conn) {
                        conn->local_ip = dst_ip;
                        conn->local_port = dst_port;
                        conn->remote_ip = src_ip;
                        conn->remote_port = src_port;
                        conn->state = TCP_LISTEN;
                    }
                }
                break;
            }
        }
    }
    
    if (conn) {
        tcp_state_machine(conn, tcp_hdr, data, data_len);
    } else if (!(tcp_hdr->flags & TCP_FLAG_RST)) {
        // Send RST for unknown connection
        terminal_writestring("TCP: No connection found, sending RST\n");
    }
}

// Socket API implementation
int tcp_socket(void) {
    for (int i = 0; i < 64; i++) {
        if (tcp_sockets[i].socket_id == -1) {
            tcp_sockets[i].socket_id = i;
            tcp_sockets[i].connection = NULL;
            tcp_sockets[i].is_listening = 0;
            return i;
        }
    }
    return -1;
}

int tcp_bind(int socket, uint16_t port) {
    if (socket < 0 || socket >= 64 || tcp_sockets[socket].socket_id == -1) {
        return -1;
    }
    
    if (!tcp_is_port_available(port)) {
        return -1;
    }
    
    tcp_connection_t* conn = tcp_create_connection();
    if (!conn) return -1;
    
    conn->local_port = port;
    conn->local_ip = get_local_ip();
    tcp_sockets[socket].connection = conn;
    
    return 0;
}

int tcp_listen(int socket, int backlog) {
    (void)backlog; // Not implemented yet
    
    if (socket < 0 || socket >= 64 || !tcp_sockets[socket].connection) {
        return -1;
    }
    
    tcp_sockets[socket].is_listening = 1;
    tcp_sockets[socket].connection->state = TCP_LISTEN;
    
    return 0;
}

int tcp_connect(int socket, uint32_t remote_ip, uint16_t remote_port) {
    if (socket < 0 || socket >= 64 || tcp_sockets[socket].socket_id == -1) {
        return -1;
    }
    
    tcp_connection_t* conn = tcp_sockets[socket].connection;
    if (!conn) {
        conn = tcp_create_connection();
        if (!conn) return -1;
        
        conn->local_port = tcp_allocate_port();
        conn->local_ip = get_local_ip();
        tcp_sockets[socket].connection = conn;
    }
    
    conn->remote_ip = remote_ip;
    conn->remote_port = remote_port;
    
    // Send SYN
    tcp_send_syn(conn);
    
    // Wait for connection (simplified - should be async)
    int timeout = 30; // 3 seconds
    while (conn->state != TCP_ESTABLISHED && timeout > 0) {
        // In real implementation, would yield to scheduler
        for (volatile int i = 0; i < 1000000; i++);
        timeout--;
    }
    
    return (conn->state == TCP_ESTABLISHED) ? 0 : -1;
}

int tcp_send(int socket, const void* data, size_t len) {
    if (socket < 0 || socket >= 64 || !tcp_sockets[socket].connection) {
        return -1;
    }
    
    tcp_connection_t* conn = tcp_sockets[socket].connection;
    if (conn->state != TCP_ESTABLISHED) {
        return -1;
    }
    
    // Send data in chunks if needed
    size_t sent = 0;
    while (sent < len) {
        size_t chunk = (len - sent > 1460) ? 1460 : (len - sent); // MSS
        tcp_send_packet(conn, TCP_FLAG_ACK | TCP_FLAG_PSH, 
                       (uint8_t*)data + sent, chunk);
        sent += chunk;
    }
    
    return sent;
}

int tcp_recv(int socket, void* buffer, size_t len) {
    if (socket < 0 || socket >= 64 || !tcp_sockets[socket].connection) {
        return -1;
    }
    
    tcp_connection_t* conn = tcp_sockets[socket].connection;
    
    // Copy data from receive buffer
    size_t to_copy = (len < conn->recv_buffer_used) ? len : conn->recv_buffer_used;
    
    for (size_t i = 0; i < to_copy; i++) {
        ((uint8_t*)buffer)[i] = conn->recv_buffer[i];
    }
    
    // Shift remaining data
    for (size_t i = 0; i < conn->recv_buffer_used - to_copy; i++) {
        conn->recv_buffer[i] = conn->recv_buffer[i + to_copy];
    }
    conn->recv_buffer_used -= to_copy;
    
    return to_copy;
}

int tcp_close(int socket) {
    if (socket < 0 || socket >= 64 || !tcp_sockets[socket].connection) {
        return -1;
    }
    
    tcp_connection_t* conn = tcp_sockets[socket].connection;
    
    if (conn->state == TCP_ESTABLISHED) {
        tcp_send_fin(conn);
        conn->state = TCP_FIN_WAIT_1;
    }
    
    // Wait for close to complete (simplified)
    int timeout = 20;
    while (conn->state != TCP_CLOSED && timeout > 0) {
        for (volatile int i = 0; i < 1000000; i++);
        timeout--;
    }
    
    tcp_destroy_connection(conn);
    tcp_sockets[socket].connection = NULL;
    tcp_sockets[socket].socket_id = -1;
    tcp_sockets[socket].is_listening = 0;
    
    return 0;
}