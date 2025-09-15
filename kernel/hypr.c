#include "../include/hypr.h"
#include "../include/vfs.h"
#include "../include/keyboard.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_setcolor(uint8_t color);
extern int keyboard_available(void);
extern char keyboard_getchar(void);
extern size_t strlen(const char* str);

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

#define VGA_COLOR_WHITE 15
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_YELLOW 14
#define VGA_COLOR_RED 4

static void hypr_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static int hypr_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void hypr_init(void) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("\n");
    terminal_writestring("========================================\n");
    terminal_writestring("    HYPR - Advanced Text Editor v1.0\n");
    terminal_writestring("    Part of MyKernel OS\n");
    terminal_writestring("    Press F1 for help, Ctrl+Q to quit\n");
    terminal_writestring("========================================\n");
    terminal_writestring("\n");
}

void hypr_clear_screen(void) {
    // Clear screen by printing many newlines
    for (int i = 0; i < 25; i++) {
        terminal_writestring("\n");
    }
}

void hypr_goto_xy(int x, int y) {
    // Simple cursor positioning (VGA doesn't support real cursor movement easily)
    // For now, we'll work with what we have
    (void)x;
    (void)y;
}

void hypr_show_help(void) {
    hypr_clear_screen();
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("HYPR Text Editor - Help\n");
    terminal_writestring("========================\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("Navigation:\n");
    terminal_writestring("  Arrow Keys    - Move cursor\n");
    terminal_writestring("  Home/End      - Beginning/End of line\n");
    terminal_writestring("  Page Up/Down  - Scroll up/down\n\n");
    
    terminal_writestring("Editing:\n");
    terminal_writestring("  Type          - Insert text\n");
    terminal_writestring("  Backspace     - Delete character before cursor\n");
    terminal_writestring("  Delete        - Delete character at cursor\n");
    terminal_writestring("  Enter         - New line\n");
    terminal_writestring("  Tab           - Insert tab (4 spaces)\n\n");
    
    terminal_writestring("File Operations:\n");
    terminal_writestring("  Ctrl+S        - Save file\n");
    terminal_writestring("  Ctrl+O        - Open file\n");
    terminal_writestring("  Ctrl+N        - New file\n");
    terminal_writestring("  Ctrl+Q        - Quit editor\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    terminal_writestring("Press any key to return to editor...\n");
    
    // Wait for key press
    while (!keyboard_available()) {
        asm("hlt");
    }
    keyboard_getchar();
}

void hypr_draw_status_line(hypr_editor_t* editor) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN));
    
    // Draw status line
    terminal_writestring(" HYPR v1.0 | File: ");
    if (editor->filename[0] != '\0') {
        terminal_writestring(editor->filename);
    } else {
        terminal_writestring("[New File]");
    }
    
    if (editor->modified) {
        terminal_writestring(" [Modified]");
    }
    
    // Position info
    terminal_writestring(" | Line: ");
    char line_str[16];
    int line_num = editor->cursor_y + 1;
    int pos = 0;
    
    if (line_num == 0) {
        line_str[pos++] = '0';
    } else {
        while (line_num > 0) {
            line_str[pos++] = '0' + (line_num % 10);
            line_num /= 10;
        }
    }
    
    // Reverse the string
    for (int i = 0; i < pos / 2; i++) {
        char temp = line_str[i];
        line_str[i] = line_str[pos - 1 - i];
        line_str[pos - 1 - i] = temp;
    }
    line_str[pos] = '\0';
    
    terminal_writestring(line_str);
    terminal_writestring(" Col: ");
    
    int col_num = editor->cursor_x + 1;
    pos = 0;
    
    if (col_num == 0) {
        line_str[pos++] = '0';
    } else {
        while (col_num > 0) {
            line_str[pos++] = '0' + (col_num % 10);
            col_num /= 10;
        }
    }
    
    // Reverse the string
    for (int i = 0; i < pos / 2; i++) {
        char temp = line_str[i];
        line_str[i] = line_str[pos - 1 - i];
        line_str[pos - 1 - i] = temp;
    }
    line_str[pos] = '\0';
    
    terminal_writestring(line_str);
    
    // Fill rest of line
    for (int i = 40; i < 80; i++) {
        terminal_writestring(" ");
    }
    
    terminal_writestring("\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

void hypr_draw_screen(hypr_editor_t* editor) {
    hypr_clear_screen();
    
    // Draw file content
    for (size_t i = 0; i < 22 && (i + editor->scroll_offset) < editor->line_count; i++) {
        size_t line_idx = i + editor->scroll_offset;
        
        // Highlight current line
        if (line_idx == editor->cursor_y) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        } else {
            terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }
        
        // Show line content
        for (size_t j = 0; j < editor->lines[line_idx].length && j < 79; j++) {
            terminal_putchar(editor->lines[line_idx].content[j]);
        }
        
        // Show cursor position
        if (line_idx == editor->cursor_y) {
            size_t cursor_pos = editor->cursor_x;
            if (cursor_pos == editor->lines[line_idx].length) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE));
                terminal_putchar(' ');
                terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            }
        }
        
        terminal_writestring("\n");
    }
    
    // Fill empty lines
    for (size_t i = editor->line_count - editor->scroll_offset; i < 22; i++) {
        terminal_writestring("~\n");
    }
    
    hypr_draw_status_line(editor);
}

int hypr_new_file(hypr_editor_t* editor, const char* filename) {
    editor->line_count = 1;
    editor->lines[0].content[0] = '\0';
    editor->lines[0].length = 0;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->scroll_offset = 0;
    editor->modified = 0;
    
    if (filename) {
        hypr_strcpy(editor->filename, filename);
    } else {
        editor->filename[0] = '\0';
    }
    
    return 0;
}

int hypr_load_file(hypr_editor_t* editor, const char* filename) {
    vfs_node_t* file = vfs_open_file(filename);
    
    if (!file || file->type != VFS_FILE) {
        // File doesn't exist, create new
        return hypr_new_file(editor, filename);
    }
    
    hypr_strcpy(editor->filename, filename);
    editor->line_count = 0;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->scroll_offset = 0;
    editor->modified = 0;
    
    // Parse file content into lines
    if (file->data && file->size > 0) {
        size_t current_line = 0;
        size_t char_pos = 0;
        
        for (size_t i = 0; i < file->size && current_line < HYPR_MAX_LINES; i++) {
            char c = file->data[i];
            
            if (c == '\n') {
                editor->lines[current_line].content[char_pos] = '\0';
                editor->lines[current_line].length = char_pos;
                current_line++;
                char_pos = 0;
            } else if (char_pos < HYPR_MAX_LINE_LEN - 1) {
                editor->lines[current_line].content[char_pos] = c;
                char_pos++;
            }
        }
        
        // Handle last line if it doesn't end with newline
        if (char_pos > 0 || current_line == 0) {
            editor->lines[current_line].content[char_pos] = '\0';
            editor->lines[current_line].length = char_pos;
            current_line++;
        }
        
        editor->line_count = current_line;
    } else {
        // Empty file
        editor->line_count = 1;
        editor->lines[0].content[0] = '\0';
        editor->lines[0].length = 0;
    }
    
    return 0;
}

int hypr_save_file(hypr_editor_t* editor) {
    if (editor->filename[0] == '\0') {
        return -1; // No filename
    }
    
    // Calculate total size needed
    size_t total_size = 0;
    for (size_t i = 0; i < editor->line_count; i++) {
        total_size += editor->lines[i].length;
        if (i < editor->line_count - 1) {
            total_size++; // For newline
        }
    }
    
    // Create buffer for file content
    char* buffer = (char*)kmalloc(total_size + 1);
    if (!buffer) {
        return -1;
    }
    
    // Build file content
    size_t pos = 0;
    for (size_t i = 0; i < editor->line_count; i++) {
        for (size_t j = 0; j < editor->lines[i].length; j++) {
            buffer[pos++] = editor->lines[i].content[j];
        }
        if (i < editor->line_count - 1) {
            buffer[pos++] = '\n';
        }
    }
    buffer[pos] = '\0';
    
    // Delete existing file if it exists
    vfs_delete_file(editor->filename);
    
    // Create new file
    vfs_node_t* file = vfs_create_file(editor->filename, (uint8_t*)buffer, total_size);
    
    kfree(buffer);
    
    if (file) {
        editor->modified = 0;
        return 0;
    }
    
    return -1;
}

void hypr_insert_char(hypr_editor_t* editor, char c) {
    if (editor->cursor_y >= HYPR_MAX_LINES) {
        return;
    }
    
    hypr_line_t* line = &editor->lines[editor->cursor_y];
    
    if (line->length < HYPR_MAX_LINE_LEN - 1) {
        // Shift characters to the right
        for (size_t i = line->length; i > editor->cursor_x; i--) {
            line->content[i] = line->content[i - 1];
        }
        
        line->content[editor->cursor_x] = c;
        line->length++;
        line->content[line->length] = '\0';
        
        editor->cursor_x++;
        editor->modified = 1;
    }
}

void hypr_delete_char(hypr_editor_t* editor) {
    if (editor->cursor_x == 0) {
        // At beginning of line - join with previous line
        if (editor->cursor_y > 0) {
            hypr_line_t* current_line = &editor->lines[editor->cursor_y];
            hypr_line_t* prev_line = &editor->lines[editor->cursor_y - 1];
            
            editor->cursor_x = prev_line->length;
            
            // Append current line to previous line
            for (size_t i = 0; i < current_line->length && prev_line->length < HYPR_MAX_LINE_LEN - 1; i++) {
                prev_line->content[prev_line->length] = current_line->content[i];
                prev_line->length++;
            }
            prev_line->content[prev_line->length] = '\0';
            
            // Shift lines up
            for (size_t i = editor->cursor_y; i < editor->line_count - 1; i++) {
                editor->lines[i] = editor->lines[i + 1];
            }
            
            editor->line_count--;
            editor->cursor_y--;
            editor->modified = 1;
        }
    } else {
        // Delete character before cursor
        hypr_line_t* line = &editor->lines[editor->cursor_y];
        
        for (size_t i = editor->cursor_x - 1; i < line->length - 1; i++) {
            line->content[i] = line->content[i + 1];
        }
        
        line->length--;
        line->content[line->length] = '\0';
        editor->cursor_x--;
        editor->modified = 1;
    }
}

void hypr_insert_newline(hypr_editor_t* editor) {
    if (editor->line_count >= HYPR_MAX_LINES) {
        return;
    }
    
    // Shift lines down
    for (size_t i = editor->line_count; i > editor->cursor_y + 1; i--) {
        editor->lines[i] = editor->lines[i - 1];
    }
    
    // Split current line
    hypr_line_t* current_line = &editor->lines[editor->cursor_y];
    hypr_line_t* new_line = &editor->lines[editor->cursor_y + 1];
    
    // Copy text after cursor to new line
    size_t remaining_chars = current_line->length - editor->cursor_x;
    for (size_t i = 0; i < remaining_chars; i++) {
        new_line->content[i] = current_line->content[editor->cursor_x + i];
    }
    new_line->length = remaining_chars;
    new_line->content[new_line->length] = '\0';
    
    // Truncate current line
    current_line->length = editor->cursor_x;
    current_line->content[current_line->length] = '\0';
    
    editor->line_count++;
    editor->cursor_y++;
    editor->cursor_x = 0;
    editor->modified = 1;
}

void hypr_move_cursor(hypr_editor_t* editor, int dx, int dy) {
    if (dy != 0) {
        int new_y = (int)editor->cursor_y + dy;
        if (new_y >= 0 && new_y < (int)editor->line_count) {
            editor->cursor_y = new_y;
            
            // Adjust x position to be within line bounds
            if (editor->cursor_x > editor->lines[editor->cursor_y].length) {
                editor->cursor_x = editor->lines[editor->cursor_y].length;
            }
            
            // Adjust scroll if needed
            if (editor->cursor_y < editor->scroll_offset) {
                editor->scroll_offset = editor->cursor_y;
            } else if (editor->cursor_y >= editor->scroll_offset + 22) {
                editor->scroll_offset = editor->cursor_y - 21;
            }
        }
    }
    
    if (dx != 0) {
        int new_x = (int)editor->cursor_x + dx;
        if (new_x >= 0 && new_x <= (int)editor->lines[editor->cursor_y].length) {
            editor->cursor_x = new_x;
        }
    }
}

void hypr_process_key(hypr_editor_t* editor, char key) {
    switch (key) {
        case KEY_UP_ARROW:
            hypr_move_cursor(editor, 0, -1);
            break;
            
        case KEY_DOWN_ARROW:
            hypr_move_cursor(editor, 0, 1);
            break;
            
        case KEY_LEFT_ARROW:
            hypr_move_cursor(editor, -1, 0);
            break;
            
        case KEY_RIGHT_ARROW:
            hypr_move_cursor(editor, 1, 0);
            break;
            
        case KEY_BACKSPACE:
            hypr_delete_char(editor);
            break;
            
        case '\n':
            hypr_insert_newline(editor);
            break;
            
        case '\t':
            // Insert 4 spaces for tab
            for (int i = 0; i < HYPR_TAB_SIZE; i++) {
                hypr_insert_char(editor, ' ');
            }
            break;
            
        case 17: // Ctrl+Q
            editor->running = 0;
            break;
            
        case 19: // Ctrl+S
            if (hypr_save_file(editor) == 0) {
                // Show save confirmation briefly
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                terminal_writestring("\n[File saved successfully]");
                
                // Wait a moment
                for (volatile int i = 0; i < 1000000; i++) {
                    asm("nop");
                }
            } else {
                terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
                terminal_writestring("\n[Error saving file]");
                
                // Wait a moment
                for (volatile int i = 0; i < 1000000; i++) {
                    asm("nop");
                }
            }
            break;
            
        default:
            if (key >= ' ' && key < 0x7F) {
                hypr_insert_char(editor, key);
            }
            break;
    }
}

void hypr_run(const char* filename) {
    hypr_editor_t editor;
    
    // Manual initialization instead of {0}
    editor.line_count = 0;
    editor.cursor_x = 0;
    editor.cursor_y = 0;
    editor.scroll_offset = 0;
    editor.filename[0] = '\0';
    editor.modified = 0;
    editor.running = 0;
    
    // Load file or create new
    if (filename) {
        hypr_load_file(&editor, filename);
    } else {
        hypr_new_file(&editor, NULL);
    }
    
    editor.running = 1;
    
    while (editor.running) {
        hypr_draw_screen(&editor);
        
        // Wait for key input
        while (!keyboard_available()) {
            asm("hlt");
        }
        
        char key = keyboard_getchar();
        
        // Special handling for F1 help
        if (key == 59) { // F1 key (scancode 59)
            hypr_show_help();
            continue;
        }
        
        // Skip null and unwanted control characters
        if (key == 0 || (key > 0 && key < ' ' && key != '\n' && key != '\t' && key != KEY_BACKSPACE && key != 17 && key != 19)) {
            continue;
        }
        
        hypr_process_key(&editor, key);
    }
    
    // Ask to save if modified
    if (editor.modified) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
        terminal_writestring("\nFile modified. Save before exit? (y/n): ");
        
        while (!keyboard_available()) {
            asm("hlt");
        }
        
        char response = keyboard_getchar();
        if (response == 'y' || response == 'Y') {
            if (editor.filename[0] != '\0') {
                hypr_save_file(&editor);
                terminal_writestring("\nFile saved.\n");
            } else {
                terminal_writestring("\nNo filename specified - file not saved.\n");
            }
        }
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("\nGoodbye from HYPR!\n");
}