#include "../include/man.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_setcolor(uint8_t color);
extern size_t strlen(const char* str);
extern int keyboard_available(void);
extern char keyboard_getchar(void);

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

#define VGA_COLOR_WHITE 15
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_YELLOW 14

static man_system_t man_system;

static void man_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static int man_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int man_strstr(const char* haystack, const char* needle) {
    if (!*needle) return 1;
    
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        
        if (!*n) return 1;
        haystack++;
    }
    
    return 0;
}

void man_init(void) {
    man_system.page_count = 0;
    
    for (int i = 0; i < MAN_MAX_PAGES; i++) {
        man_system.pages[i].active = 0;
        man_system.pages[i].name[0] = '\0';
        man_system.pages[i].section[0] = '\0';
        man_system.pages[i].content[0] = '\0';
    }
    
    // Create built-in manual pages
    man_create_builtin_pages();
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("MAN system initialized with ");
    // Simple number to string conversion
    char count_str[16];
    int count = man_system.page_count;
    int pos = 0;
    
    if (count == 0) {
        count_str[pos++] = '0';
    } else {
        while (count > 0) {
            count_str[pos++] = '0' + (count % 10);
            count /= 10;
        }
    }
    
    // Reverse the string
    for (int i = 0; i < pos / 2; i++) {
        char temp = count_str[i];
        count_str[i] = count_str[pos - 1 - i];
        count_str[pos - 1 - i] = temp;
    }
    count_str[pos] = '\0';
    
    terminal_writestring(count_str);
    terminal_writestring(" manual pages\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

int man_add_page(const char* name, const char* section, const char* content) {
    if (man_system.page_count >= MAN_MAX_PAGES) {
        return -1; // No space
    }
    
    // Check if page already exists
    for (int i = 0; i < MAN_MAX_PAGES; i++) {
        if (man_system.pages[i].active && 
            man_strcmp(man_system.pages[i].name, name) == 0) {
            return -2; // Already exists
        }
    }
    
    // Find empty slot
    for (int i = 0; i < MAN_MAX_PAGES; i++) {
        if (!man_system.pages[i].active) {
            man_strcpy(man_system.pages[i].name, name);
            man_strcpy(man_system.pages[i].section, section);
            man_strcpy(man_system.pages[i].content, content);
            man_system.pages[i].active = 1;
            man_system.page_count++;
            return 0;
        }
    }
    
    return -1;
}

void man_show_page(const char* name) {
    // Find the page
    man_page_t* page = NULL;
    for (int i = 0; i < MAN_MAX_PAGES; i++) {
        if (man_system.pages[i].active && 
            man_strcmp(man_system.pages[i].name, name) == 0) {
            page = &man_system.pages[i];
            break;
        }
    }
    
    if (!page) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        terminal_writestring("No manual entry for ");
        terminal_writestring(name);
        terminal_writestring("\n");
        return;
    }
    
    // Display the manual page with paging
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring(page->name);
    terminal_writestring("(");
    terminal_writestring(page->section);
    terminal_writestring(")");
    
    // Fill the rest of the line
    int title_len = strlen(page->name) + strlen(page->section) + 2;
    for (int i = title_len; i < 40; i++) {
        terminal_writestring(" ");
    }
    
    terminal_writestring(page->name);
    terminal_writestring("(");
    terminal_writestring(page->section);
    terminal_writestring(")\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("\n");
    
    // Display content with simple paging
    const char* content = page->content;
    int lines_shown = 0;
    
    while (*content) {
        // Show one line
        while (*content && *content != '\n') {
            terminal_putchar(*content);
            content++;
        }
        
        if (*content == '\n') {
            terminal_putchar('\n');
            content++;
            lines_shown++;
            
            // Page after 20 lines
            if (lines_shown >= 20) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
                terminal_writestring("-- Press any key to continue, 'q' to quit --");
                terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
                
                while (!keyboard_available()) {
                    asm("hlt");
                }
                
                char key = keyboard_getchar();
                if (key == 'q' || key == 'Q') {
                    terminal_writestring("\n");
                    return;
                }
                
                // Clear the prompt line
                terminal_writestring("\r                                                \r");
                lines_shown = 0;
            }
        }
    }
    
    terminal_writestring("\n");
}

void man_list_pages(void) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Available manual pages:\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    for (int i = 0; i < MAN_MAX_PAGES; i++) {
        if (man_system.pages[i].active) {
            terminal_writestring("  ");
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            terminal_writestring(man_system.pages[i].name);
            terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            terminal_writestring("(");
            terminal_writestring(man_system.pages[i].section);
            terminal_writestring(")\n");
        }
    }
}

int man_search(const char* keyword) {
    int found = 0;
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Searching for: ");
    terminal_writestring(keyword);
    terminal_writestring("\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    for (int i = 0; i < MAN_MAX_PAGES; i++) {
        if (man_system.pages[i].active) {
            // Search in name and content
            if (man_strstr(man_system.pages[i].name, keyword) || 
                man_strstr(man_system.pages[i].content, keyword)) {
                
                terminal_writestring("  ");
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                terminal_writestring(man_system.pages[i].name);
                terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
                terminal_writestring("(");
                terminal_writestring(man_system.pages[i].section);
                terminal_writestring(")\n");
                found++;
            }
        }
    }
    
    if (found == 0) {
        terminal_writestring("No matches found.\n");
    }
    
    return found;
}

void man_create_builtin_pages(void) {
    // BSH Shell commands
    man_add_page("bsh", "1", 
        "NAME\n"
        "    bsh - Basic Shell for MyKernel OS\n\n"
        "SYNOPSIS\n"
        "    Interactive command shell\n\n"
        "DESCRIPTION\n"
        "    BSH is the default shell for MyKernel OS. It provides\n"
        "    a command-line interface for file operations, system\n"
        "    information, and program execution.\n\n"
        "FEATURES\n"
        "    - Command history with arrow key navigation\n"
        "    - Tab completion\n"
        "    - Built-in file operations\n"
        "    - Color-coded output\n\n"
        "BUILT-IN COMMANDS\n"
        "    help     - Show available commands\n"
        "    clear    - Clear screen\n"
        "    exit     - Exit shell\n"
        "    history  - Show command history\n\n"
        "SEE ALSO\n"
        "    ls(1), cd(1), mkdir(1), hypr(1)\n");
    
    man_add_page("ls", "1",
        "NAME\n"
        "    ls - list directory contents\n\n"
        "SYNOPSIS\n"
        "    ls\n\n"
        "DESCRIPTION\n"
        "    List information about files and directories in the\n"
        "    current directory.\n\n"
        "EXAMPLES\n"
        "    ls          List current directory contents\n\n"
        "SEE ALSO\n"
        "    cd(1), pwd(1), mkdir(1)\n");
    
    man_add_page("cd", "1",
        "NAME\n"
        "    cd - change directory\n\n"
        "SYNOPSIS\n"
        "    cd [directory]\n\n"
        "DESCRIPTION\n"
        "    Change the current working directory to the specified\n"
        "    directory. If no directory is specified, change to\n"
        "    the root directory.\n\n"
        "EXAMPLES\n"
        "    cd /        Change to root directory\n"
        "    cd docs     Change to docs subdirectory\n\n"
        "SEE ALSO\n"
        "    ls(1), pwd(1), mkdir(1)\n");
    
    man_add_page("mkdir", "1",
        "NAME\n"
        "    mkdir - create directories\n\n"
        "SYNOPSIS\n"
        "    mkdir directory\n\n"
        "DESCRIPTION\n"
        "    Create the specified directory.\n\n"
        "EXAMPLES\n"
        "    mkdir docs     Create a directory named 'docs'\n\n"
        "SEE ALSO\n"
        "    rmdir(1), ls(1), cd(1)\n");
    
    man_add_page("hypr", "1",
        "NAME\n"
        "    hypr - advanced text editor\n\n"
        "SYNOPSIS\n"
        "    hypr [filename]\n\n"
        "DESCRIPTION\n"
        "    HYPR is a full-featured text editor for MyKernel OS.\n"
        "    It supports file editing, syntax highlighting, and\n"
        "    advanced navigation features.\n\n"
        "KEY BINDINGS\n"
        "    Arrow Keys    - Move cursor\n"
        "    Ctrl+S        - Save file\n"
        "    Ctrl+Q        - Quit editor\n"
        "    Ctrl+O        - Open file\n"
        "    F1            - Show help\n"
        "    Backspace     - Delete character\n"
        "    Enter         - New line\n"
        "    Tab           - Insert 4 spaces\n\n"
        "EXAMPLES\n"
        "    hypr           Start with new file\n"
        "    hypr test.txt  Edit existing file\n\n"
        "SEE ALSO\n"
        "    bsh(1), touch(1)\n");
    
    man_add_page("man", "1",
        "NAME\n"
        "    man - display manual pages\n\n"
        "SYNOPSIS\n"
        "    man [command]\n"
        "    man -k keyword\n"
        "    man -l\n\n"
        "DESCRIPTION\n"
        "    Display manual pages for commands and programs.\n\n"
        "OPTIONS\n"
        "    -k keyword    Search for keyword in manual pages\n"
        "    -l            List all available manual pages\n\n"
        "EXAMPLES\n"
        "    man ls        Show manual for ls command\n"
        "    man -k file   Search for 'file' in all pages\n"
        "    man -l        List all manual pages\n\n"
        "SEE ALSO\n"
        "    help(1), bsh(1)\n");
    
    man_add_page("ps", "1",
        "NAME\n"
        "    ps - display running processes\n\n"
        "SYNOPSIS\n"
        "    ps\n\n"
        "DESCRIPTION\n"
        "    Display information about currently running processes\n"
        "    in the system.\n\n"
        "SEE ALSO\n"
        "    version(1), mem(1)\n");
    
    man_add_page("version", "1",
        "NAME\n"
        "    version - display system version\n\n"
        "SYNOPSIS\n"
        "    version\n\n"
        "DESCRIPTION\n"
        "    Display MyKernel OS version information and features.\n\n"
        "SEE ALSO\n"
        "    ps(1), mem(1)\n");
}