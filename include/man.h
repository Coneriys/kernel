#ifndef MAN_H
#define MAN_H

#include <stdint.h>
#include <stddef.h>

#define MAN_MAX_PAGES 64
#define MAN_MAX_NAME_LEN 32
#define MAN_MAX_CONTENT_LEN 2048
#define MAN_MAX_SECTION_LEN 16

typedef struct {
    char name[MAN_MAX_NAME_LEN];
    char section[MAN_MAX_SECTION_LEN];
    char content[MAN_MAX_CONTENT_LEN];
    int active;
} man_page_t;

typedef struct {
    man_page_t pages[MAN_MAX_PAGES];
    int page_count;
} man_system_t;

// MAN system functions
void man_init(void);
int man_add_page(const char* name, const char* section, const char* content);
void man_show_page(const char* name);
void man_list_pages(void);
int man_search(const char* keyword);

// Built-in manual pages
void man_create_builtin_pages(void);

#endif