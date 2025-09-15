#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h"

// TCP header flags
#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_URG  0x20

// TCP states
typedef enum {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT
} tcp_state_t;

// TCP header structure
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset;  // 4 bits offset + 4 bits reserved
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

// TCP pseudo header for checksum
typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t  zero;
    uint8_t  protocol;
    uint16_t tcp_length;
} __attribute__((packed)) tcp_pseudo_header_t;

// TCP connection structure
typedef struct tcp_connection {
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    
    tcp_state_t state;
    
    // Sequence numbers
    uint32_t send_seq;
    uint32_t send_ack;
    uint32_t recv_seq;
    uint32_t recv_ack;
    
    // Window management
    uint16_t send_window;
    uint16_t recv_window;
    
    // Buffers
    uint8_t* send_buffer;
    uint8_t* recv_buffer;
    size_t send_buffer_size;
    size_t recv_buffer_size;
    size_t send_buffer_used;
    size_t recv_buffer_used;
    
    // Connection tracking
    struct tcp_connection* next;
} tcp_connection_t;

// TCP socket structure
typedef struct {
    tcp_connection_t* connection;
    int socket_id;
    int is_listening;
} tcp_socket_t;

// TCP functions
void tcp_init(void);
void tcp_handle_packet(uint8_t* packet, size_t len, uint32_t src_ip, uint32_t dst_ip);
uint16_t tcp_checksum(tcp_header_t* tcp_hdr, uint8_t* data, size_t data_len, uint32_t src_ip, uint32_t dst_ip);

// Socket API
int tcp_socket(void);
int tcp_bind(int socket, uint16_t port);
int tcp_listen(int socket, int backlog);
int tcp_accept(int socket, uint32_t* remote_ip, uint16_t* remote_port);
int tcp_connect(int socket, uint32_t remote_ip, uint16_t remote_port);
int tcp_send(int socket, const void* data, size_t len);
int tcp_recv(int socket, void* buffer, size_t len);
int tcp_close(int socket);

// Connection management
tcp_connection_t* tcp_create_connection(void);
void tcp_destroy_connection(tcp_connection_t* conn);
tcp_connection_t* tcp_find_connection(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port);

// State machine
void tcp_state_machine(tcp_connection_t* conn, tcp_header_t* tcp_hdr, uint8_t* data, size_t data_len);
void tcp_send_packet(tcp_connection_t* conn, uint8_t flags, uint8_t* data, size_t data_len);
void tcp_send_syn(tcp_connection_t* conn);
void tcp_send_syn_ack(tcp_connection_t* conn);
void tcp_send_ack(tcp_connection_t* conn);
void tcp_send_fin(tcp_connection_t* conn);
void tcp_send_rst(tcp_connection_t* conn);

// Timer functions
void tcp_timer_tick(void);
void tcp_retransmit_check(void);

// Utility functions
uint16_t tcp_allocate_port(void);
int tcp_is_port_available(uint16_t port);

#endif