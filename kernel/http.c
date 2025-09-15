#include "../include/http.h"
#include "../include/tcp.h"
#include "../include/net.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);
extern size_t strlen(const char* str);

// Simple string functions
static void http_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static int http_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static char* http_strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        
        if (!*n) return (char*)haystack;
    }
    
    return NULL;
}

static int http_atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    if (*str == '-') {
        sign = -1;
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

// Convert IP string to uint32_t
static uint32_t http_parse_ip(const char* ip_str) {
    uint32_t ip = 0;
    int octet = 0;
    int value = 0;
    
    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9') {
            value = value * 10 + (*ip_str - '0');
        } else if (*ip_str == '.') {
            ip = (ip << 8) | (value & 0xFF);
            value = 0;
            octet++;
            if (octet >= 3) break;
        }
        ip_str++;
    }
    
    // Last octet
    ip = (ip << 8) | (value & 0xFF);
    
    return ip;
}

int http_parse_url(const char* url, char* host, uint16_t* port, char* path) {
    // Default values
    *port = 80;
    http_strcpy(path, "/");
    
    // Skip http://
    if (http_strstr(url, "http://") == url) {
        url += 7;
    }
    
    // Extract host
    int i = 0;
    while (*url && *url != ':' && *url != '/') {
        host[i++] = *url++;
    }
    host[i] = '\0';
    
    // Extract port if present
    if (*url == ':') {
        url++;
        *port = 0;
        while (*url >= '0' && *url <= '9') {
            *port = *port * 10 + (*url - '0');
            url++;
        }
    }
    
    // Extract path
    if (*url == '/') {
        http_strcpy(path, url);
    }
    
    return 0;
}

int http_get(const char* host, uint16_t port, const char* path, http_response_t* response) {
    terminal_writestring("HTTP GET ");
    terminal_writestring(host);
    terminal_writestring(path);
    terminal_writestring("\n");
    
    // For now, use hardcoded IP (would need DNS resolver)
    uint32_t server_ip = http_parse_ip("93.184.216.34"); // example.com
    
    // Create TCP socket
    int sock = tcp_socket();
    if (sock < 0) {
        terminal_writestring("Failed to create socket\n");
        return -1;
    }
    
    // Connect to server
    terminal_writestring("Connecting to server...\n");
    if (tcp_connect(sock, server_ip, port) < 0) {
        terminal_writestring("Failed to connect\n");
        tcp_close(sock);
        return -1;
    }
    
    terminal_writestring("Connected!\n");
    
    // Build HTTP request
    char request[512];
    char* p = request;
    
    // Request line
    http_strcpy(p, "GET ");
    p += strlen(p);
    http_strcpy(p, path);
    p += strlen(p);
    http_strcpy(p, " HTTP/1.0\r\n");
    p += strlen(p);
    
    // Host header
    http_strcpy(p, "Host: ");
    p += strlen(p);
    http_strcpy(p, host);
    p += strlen(p);
    http_strcpy(p, "\r\n");
    p += strlen(p);
    
    // User-Agent
    http_strcpy(p, "User-Agent: MyKernel/1.0\r\n");
    p += strlen(p);
    
    // Connection close
    http_strcpy(p, "Connection: close\r\n");
    p += strlen(p);
    
    // End of headers
    http_strcpy(p, "\r\n");
    
    // Send request
    terminal_writestring("Sending HTTP request...\n");
    int sent = tcp_send(sock, request, strlen(request));
    if (sent < 0) {
        terminal_writestring("Failed to send request\n");
        tcp_close(sock);
        return -1;
    }
    
    // Receive response
    terminal_writestring("Waiting for response...\n");
    char* buffer = (char*)kmalloc(8192);
    if (!buffer) {
        tcp_close(sock);
        return -1;
    }
    
    int total_received = 0;
    int received;
    
    while ((received = tcp_recv(sock, buffer + total_received, 8192 - total_received - 1)) > 0) {
        total_received += received;
        if (total_received >= 8191) break;
    }
    
    buffer[total_received] = '\0';
    
    // Parse response
    response->headers = buffer;
    response->body = http_strstr(buffer, "\r\n\r\n");
    
    if (response->body) {
        response->body += 4; // Skip \r\n\r\n
        response->body_length = total_received - (response->body - buffer);
        
        // Parse status code
        char* status_line = buffer;
        char* space = http_strstr(status_line, " ");
        if (space) {
            response->status_code = http_atoi(space + 1);
        }
    }
    
    terminal_writestring("Response received!\n");
    
    tcp_close(sock);
    return 0;
}

void http_free_response(http_response_t* response) {
    if (response->headers) {
        kfree(response->headers);
        response->headers = NULL;
        response->body = NULL;
    }
}