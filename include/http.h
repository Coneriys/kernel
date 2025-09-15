#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <stddef.h>

// HTTP response structure
typedef struct {
    int status_code;
    char* headers;
    char* body;
    size_t body_length;
} http_response_t;

// HTTP functions
int http_get(const char* host, uint16_t port, const char* path, http_response_t* response);
void http_free_response(http_response_t* response);

// URL parsing
int http_parse_url(const char* url, char* host, uint16_t* port, char* path);

#endif