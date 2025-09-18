// Simplified BSH for testing modern video/GUI system
#include "../include/bsh.h"
#include "../include/video.h"
#include "../include/disk.h"
#include "../include/fat32.h"
#include "../include/installer.h"
// GUI removed - will be rewritten

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_setcolor(uint8_t color);
extern void terminal_backspace(void);
extern int keyboard_available(void);
extern char keyboard_getchar(void);
extern size_t strlen(const char* str);

// Simple commands for testing
typedef struct {
    const char* name;
    const char* description;
    int (*handler)(const char* args);
} simple_command_t;

int cmd_help(const char* args);
int cmd_text(const char* args);
int cmd_video(const char* args);
int cmd_exit(const char* args);
int cmd_modern_font_demo(const char* args);
int cmd_gui2(const char* args);
int cmd_disks(const char* args);
int cmd_format(const char* args);
int cmd_install(const char* args);

static simple_command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"text", "Return to text mode", cmd_text},
    {"video", "Show video information", cmd_video},
    {"fontdemo", "Demonstrate modern SF Pro font system", cmd_modern_font_demo},
    {"gui2", "Launch new GUI system", cmd_gui2},
    {"disks", "Show disk information", cmd_disks},
    {"format", "Format disk with FAT32 (format 0)", cmd_format},
    {"install", "Install ByteOS to disk (install 0)", cmd_install},
    {"exit", "Exit shell", cmd_exit},
    {NULL, NULL, NULL}
};

static int shell_running = 1;

int cmd_help(const char* args) {
    (void)args;
    terminal_writestring("Available commands:\n");
    for (int i = 0; commands[i].name; i++) {
        terminal_writestring("  ");
        terminal_writestring(commands[i].name);
        terminal_writestring(" - ");
        terminal_writestring(commands[i].description);
        terminal_writestring("\n");
    }
    return 0;
}

// GUI functions removed - will be rewritten from scratch

int cmd_text(const char* args) {
    (void)args;
    
    if (video_set_mode(VIDEO_MODE_TEXT)) {
        extern void terminal_initialize(void);
        terminal_initialize();
        terminal_writestring("Returned to text mode\n");
        terminal_writestring("BSH> ");
        return 0;
    } else {
        terminal_writestring("Failed to switch to text mode\n");
        return 1;
    }
}

int cmd_video(const char* args) {
    (void)args;
    
    video_driver_t* driver = video_get_driver();
    if (!driver) {
        terminal_writestring("No video driver available\n");
        return 1;
    }
    
    terminal_writestring("Video System Information:\n");
    terminal_writestring("Current mode: ");
    
    switch (driver->current_mode) {
        case VIDEO_MODE_TEXT:
            terminal_writestring("Text Mode\n");
            break;
        case VIDEO_MODE_HD_1080P:
            terminal_writestring("Full HD 1080p (1920x1080)\n");
            break;
        case VIDEO_MODE_HD_720P:
            terminal_writestring("HD 720p (1280x720)\n");
            break;
        case VIDEO_MODE_VESA_1024x768:
            terminal_writestring("XGA (1024x768)\n");
            break;
        case VIDEO_MODE_VGA_FALLBACK:
            terminal_writestring("VGA Fallback (320x200)\n");
            break;
        default:
            terminal_writestring("Unknown\n");
            break;
    }
    
    return 0;
}

int cmd_exit(const char* args) {
    (void)args;
    shell_running = 0;
    terminal_writestring("Goodbye!\n");
    return 0;
}

// Simple string comparison
static int simple_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

// Simple command parsing
static int execute_command(const char* input) {
    // Skip leading spaces
    while (*input == ' ') input++;
    
    // Find command name
    const char* cmd_end = input;
    while (*cmd_end && *cmd_end != ' ') cmd_end++;
    
    // Find arguments
    const char* args = cmd_end;
    while (*args == ' ') args++;
    
    // Look for command
    for (int i = 0; commands[i].name; i++) {
        size_t cmd_len = cmd_end - input;
        if (strlen(commands[i].name) == cmd_len &&
            simple_strcmp(commands[i].name, input) == 0) {
            return commands[i].handler(args);
        }
    }
    
    terminal_writestring("Unknown command: ");
    for (const char* p = input; p < cmd_end; p++) {
        terminal_putchar(*p);
    }
    terminal_writestring("\nType 'help' for available commands.\n");
    return 1;
}

void bsh_init(void) {
    terminal_writestring("MyKernel Simple Shell v2.0\n");
    terminal_writestring("Modern HD Graphics System Ready\n");
    terminal_writestring("Type 'help' for commands (GUI system will be rewritten)\n");
}

void bsh_run(void) {
    char input_buffer[256];
    int input_pos = 0;
    
    terminal_writestring("BSH> ");
    
    while (shell_running) {
        if (keyboard_available()) {
            char c = keyboard_getchar();
            
            if (c == '\n' || c == '\r') {
                terminal_putchar('\n');
                
                if (input_pos > 0) {
                    input_buffer[input_pos] = '\0';
                    execute_command(input_buffer);
                    input_pos = 0;
                }
                
                if (shell_running) {
                    terminal_writestring("BSH> ");
                }
            } else if (c == '\b' || c == 127) { // Backspace
                if (input_pos > 0) {
                    input_pos--;
                    terminal_backspace();
                }
            } else if (c >= 32 && c < 127 && input_pos < 255) { // Printable characters
                input_buffer[input_pos++] = c;
                terminal_putchar(c);
            }
        }
        
        // Yield CPU
        asm("hlt");
    }
}

// Compositor function removed - will be rewritten

int cmd_modern_font_demo(const char* args) {
    (void)args;
    
    terminal_writestring("Starting modern font demonstration...\n");
    terminal_writestring("Switching to graphics mode for font rendering\n");
    
    // Switch to graphics mode first
    if (!video_set_mode(VIDEO_MODE_HD_720P) && 
        !video_set_mode(VIDEO_MODE_VESA_1024x768) &&
        !video_set_mode(VIDEO_MODE_VGA_FALLBACK)) {
        terminal_writestring("ERROR: Could not set graphics mode for font demo\n");
        return 1;
    }
    
    // Give the video mode time to stabilize
    for (volatile int i = 0; i < 100000; i++);
    
    terminal_writestring("Graphics mode set. Running font demonstration...\n");
    
    // Run the font demonstration
    terminal_writestring("Font demo removed");
    
    terminal_writestring("Font demonstration completed. Press any key to return to text mode...\n");
    
    // Wait for keypress
    while (!keyboard_available()) {
        // Wait for input
        asm("hlt");
    }
    keyboard_getchar(); // Consume the character
    
    // Return to text mode
    video_set_mode(VIDEO_MODE_TEXT);
    extern void terminal_initialize(void);
    terminal_initialize();
    
    terminal_writestring("Returned to text mode.\n");
    return 0;
}

int cmd_gui2(const char* args) {
    (void)args;
    
    terminal_writestring("Starting new GUI2 system...\n");
    terminal_writestring("Switching to graphics mode...\n");
    
    // Switch to graphics mode first
    if (!video_set_mode(VIDEO_MODE_HD_720P) && 
        !video_set_mode(VIDEO_MODE_VESA_1024x768) &&
        !video_set_mode(VIDEO_MODE_VGA_FALLBACK)) {
        terminal_writestring("ERROR: Could not set graphics mode for GUI2\n");
        return 1;
    }
    
    // Get video driver info
    extern video_driver_t* video_get_driver(void);
    video_driver_t* driver = video_get_driver();
    if (!driver) {
        terminal_writestring("ERROR: No video driver available\n");
        return 1;
    }
    
    terminal_writestring("Graphics mode set successfully\n");
    terminal_writestring("Launching GUI2 window manager...\n");
    
    // Launch the new GUI system
    extern int gui2_main_loop(void);
    return gui2_main_loop();
}

int cmd_disks(const char* args) {
    (void)args;
    
    terminal_writestring("Disk Information:\n");
    terminal_writestring("=================\n");
    
    for (int i = 0; i < 4; i++) {
        disk_info_t* disk = disk_get_info(i);
        if (disk && disk->present) {
            terminal_writestring("Disk ");
            terminal_putchar('0' + i);
            terminal_writestring(": ");
            
            // Show disk type
            switch (disk->type) {
                case DISK_TYPE_ATA_PATA:
                    terminal_writestring("ATA/PATA ");
                    break;
                case DISK_TYPE_ATA_SATA:
                    terminal_writestring("ATA/SATA ");
                    break;
                case DISK_TYPE_ATAPI:
                    terminal_writestring("ATAPI ");
                    break;
                default:
                    terminal_writestring("Unknown ");
                    break;
            }
            
            // Show capacity
            uint64_t size_mb = (disk->sectors * disk->sector_size) / (1024 * 1024);
            terminal_writestring("(");
            // Simple number printing
            char size_str[16];
            int pos = 0;
            uint64_t temp = size_mb;
            if (temp == 0) {
                size_str[pos++] = '0';
            } else {
                while (temp > 0) {
                    size_str[pos++] = '0' + (temp % 10);
                    temp /= 10;
                }
                // Reverse the string
                for (int j = 0; j < pos / 2; j++) {
                    char swap = size_str[j];
                    size_str[j] = size_str[pos - 1 - j];
                    size_str[pos - 1 - j] = swap;
                }
            }
            size_str[pos] = '\0';
            
            terminal_writestring(size_str);
            terminal_writestring(" MB)\n");
            
            // Show model if available
            terminal_writestring("  Model: ");
            // Print first 20 chars of model, trimming spaces
            for (int j = 0; j < 20 && disk->model[j]; j++) {
                if (disk->model[j] != ' ' || j == 0) {
                    terminal_putchar(disk->model[j]);
                }
            }
            terminal_writestring("\n");
        }
    }
    
    terminal_writestring("\nDisk detection completed.\n");
    return 0;
}

int cmd_format(const char* args) {
    (void)args;
    
    terminal_writestring("FAT32 Disk Formatter\n");
    terminal_writestring("====================\n");
    
    // Simple argument parsing - just get disk number
    int disk_id = 0;
    if (args && *args >= '0' && *args <= '3') {
        disk_id = *args - '0';
    }
    
    // Check if disk exists
    disk_info_t* disk = disk_get_info(disk_id);
    if (!disk || !disk->present) {
        terminal_writestring("Error: Disk ");
        terminal_putchar('0' + disk_id);
        terminal_writestring(" not found!\n");
        return 1;
    }
    
    // Show disk info
    terminal_writestring("Formatting disk ");
    terminal_putchar('0' + disk_id);
    terminal_writestring(":\n");
    
    uint64_t size_mb = (disk->sectors * disk->sector_size) / (1024 * 1024);
    terminal_writestring("  Size: ");
    
    // Print size
    char size_str[16];
    int pos = 0;
    uint64_t temp = size_mb;
    if (temp == 0) {
        size_str[pos++] = '0';
    } else {
        while (temp > 0) {
            size_str[pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        // Reverse the string
        for (int j = 0; j < pos / 2; j++) {
            char swap = size_str[j];
            size_str[j] = size_str[pos - 1 - j];
            size_str[pos - 1 - j] = swap;
        }
    }
    size_str[pos] = '\0';
    
    terminal_writestring(size_str);
    terminal_writestring(" MB\n");
    
    terminal_writestring("\nWarning: This will erase all data on the disk!\n");
    terminal_writestring("Starting FAT32 format...\n");
    
    // Format the entire disk as FAT32
    uint32_t start_sector = 0;
    uint32_t total_sectors = (uint32_t)disk->sectors;
    
    if (fat32_format_disk(disk_id, start_sector, total_sectors, "ByteOS")) {
        terminal_writestring("Format completed successfully!\n");
        terminal_writestring("Filesystem: FAT32\n");
        terminal_writestring("Volume label: ByteOS\n");
        return 0;
    } else {
        terminal_writestring("Format failed!\n");
        return 1;
    }
}

int cmd_install(const char* args) {
    (void)args;
    
    terminal_writestring("ByteOS System Installer\n");
    terminal_writestring("=======================\n");
    
    // Parse disk argument
    int disk_id = 0;
    if (args && *args >= '0' && *args <= '3') {
        disk_id = *args - '0';
    }
    
    // Check if disk exists
    disk_info_t* disk = disk_get_info(disk_id);
    if (!disk || !disk->present) {
        terminal_writestring("Error: Disk ");
        terminal_putchar('0' + disk_id);
        terminal_writestring(" not found!\n");
        return 1;
    }
    
    // Show disk info
    terminal_writestring("Installing ByteOS to disk ");
    terminal_putchar('0' + disk_id);
    terminal_writestring(":\n");
    
    uint64_t size_mb = (disk->sectors * disk->sector_size) / (1024 * 1024);
    terminal_writestring("  Size: ");
    
    // Print size
    char size_str[16];
    int pos = 0;
    uint64_t temp = size_mb;
    if (temp == 0) {
        size_str[pos++] = '0';
    } else {
        while (temp > 0) {
            size_str[pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        // Reverse the string
        for (int j = 0; j < pos / 2; j++) {
            char swap = size_str[j];
            size_str[j] = size_str[pos - 1 - j];
            size_str[pos - 1 - j] = swap;
        }
    }
    size_str[pos] = '\0';
    
    terminal_writestring(size_str);
    terminal_writestring(" MB\n\n");
    
    // Initialize installer
    if (!installer_init()) {
        terminal_writestring("Error: Failed to initialize installer\n");
        return 1;
    }
    
    // Setup installation config
    install_config_t config = {0};
    config.target_disk = disk_id;
    config.volume_label = "ByteOS";
    config.files = NULL;
    config.file_count = 0;
    
    terminal_writestring("Warning: This will erase all data on the disk!\n");
    terminal_writestring("Starting installation...\n\n");
    
    // Start installation
    if (installer_start(&config)) {
        terminal_writestring("Installation completed successfully!\n");
        terminal_writestring("System Status:\n");
        terminal_writestring("  Filesystem: FAT32\n");
        terminal_writestring("  Bootloader: Installed\n");
        terminal_writestring("  Kernel: Installed\n");
        terminal_writestring("  System Files: Installed\n");
        terminal_writestring("\nByteOS is ready to boot from disk ");
        terminal_putchar('0' + disk_id);
        terminal_writestring("!\n");
        return 0;
    } else {
        terminal_writestring("Installation failed: ");
        terminal_writestring(installer_get_status_message());
        terminal_writestring("\n");
        return 1;
    }
}