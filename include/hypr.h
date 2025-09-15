#ifndef HYPR_H
#define HYPR_H

#include <stdint.h>
#include <stddef.h>
#include "../include/vfs.h"

#define HYPR_MAX_LINES 100
#define HYPR_MAX_LINE_LEN 80
#define HYPR_TAB_SIZE 4

typedef struct {
    char content[HYPR_MAX_LINE_LEN];
    size_t length;
} hypr_line_t;

typedef struct {
    hypr_line_t lines[HYPR_MAX_LINES];
    size_t line_count;
    size_t cursor_x;
    size_t cursor_y;
    size_t scroll_offset;
    char filename[VFS_MAX_NAME_LEN];
    int modified;
    int running;
} hypr_editor_t;

// Editor functions
void hypr_init(void);
void hypr_run(const char* filename);
void hypr_draw_screen(hypr_editor_t* editor);
void hypr_draw_status_line(hypr_editor_t* editor);
void hypr_process_key(hypr_editor_t* editor, char key);

// File operations
int hypr_load_file(hypr_editor_t* editor, const char* filename);
int hypr_save_file(hypr_editor_t* editor);
int hypr_new_file(hypr_editor_t* editor, const char* filename);

// Editor operations
void hypr_insert_char(hypr_editor_t* editor, char c);
void hypr_delete_char(hypr_editor_t* editor);
void hypr_insert_newline(hypr_editor_t* editor);
void hypr_move_cursor(hypr_editor_t* editor, int dx, int dy);

// Utility functions
void hypr_clear_screen(void);
void hypr_goto_xy(int x, int y);
void hypr_show_help(void);

#endif