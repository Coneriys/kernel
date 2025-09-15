#include "../include/vfs.h"
#include "../include/memory.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);
extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern size_t strlen(const char* str);

static vfs_context_t vfs_ctx;

static void vfs_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static int vfs_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int vfs_strncmp(const char* s1, const char* s2, size_t n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (n == (size_t)-1) ? 0 : *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void vfs_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void vfs_init(void) {
    // Create root directory
    vfs_ctx.root_dir = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!vfs_ctx.root_dir) {
        terminal_writestring("VFS: Failed to allocate root directory\n");
        return;
    }
    
    vfs_strcpy(vfs_ctx.root_dir->name, "/");
    vfs_ctx.root_dir->type = VFS_DIRECTORY;
    vfs_ctx.root_dir->size = 0;
    vfs_ctx.root_dir->data = NULL;
    vfs_ctx.root_dir->parent = NULL;
    vfs_ctx.root_dir->child_count = 0;
    vfs_ctx.root_dir->creation_time = 0;
    
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        vfs_ctx.root_dir->children[i] = NULL;
    }
    
    vfs_ctx.current_dir = vfs_ctx.root_dir;
    vfs_strcpy(vfs_ctx.current_path, "/");
    
    // Create some default directories
    vfs_mkdir("home");
    vfs_mkdir("bin");
    vfs_mkdir("etc");
    vfs_mkdir("tmp");
    
    terminal_writestring("VFS: Virtual File System initialized\n");
}

vfs_node_t* vfs_find_child(vfs_node_t* parent, const char* name) {
    if (!parent || parent->type != VFS_DIRECTORY) {
        return NULL;
    }
    
    for (size_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] && vfs_strcmp(parent->children[i]->name, name) == 0) {
            return parent->children[i];
        }
    }
    
    return NULL;
}

vfs_node_t* vfs_mkdir(const char* name) {
    if (!name || !vfs_is_valid_name(name)) {
        return NULL;
    }
    
    // Check if directory already exists
    if (vfs_find_child(vfs_ctx.current_dir, name)) {
        return NULL; // Already exists
    }
    
    // Check if we have space for more children
    if (vfs_ctx.current_dir->child_count >= VFS_MAX_FILES) {
        return NULL; // Directory full
    }
    
    // Create new directory
    vfs_node_t* new_dir = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!new_dir) {
        return NULL;
    }
    
    vfs_strcpy(new_dir->name, name);
    new_dir->type = VFS_DIRECTORY;
    new_dir->size = 0;
    new_dir->data = NULL;
    new_dir->parent = vfs_ctx.current_dir;
    new_dir->child_count = 0;
    new_dir->creation_time = 0; // TODO: Add proper timestamp
    
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        new_dir->children[i] = NULL;
    }
    
    // Add to parent directory
    vfs_ctx.current_dir->children[vfs_ctx.current_dir->child_count++] = new_dir;
    
    return new_dir;
}

vfs_node_t* vfs_chdir(const char* path) {
    if (!path) {
        return vfs_ctx.current_dir;
    }
    
    vfs_node_t* target_dir = NULL;
    
    if (vfs_strcmp(path, "/") == 0) {
        // Go to root
        target_dir = vfs_ctx.root_dir;
    } else if (vfs_strcmp(path, "..") == 0) {
        // Go to parent
        if (vfs_ctx.current_dir->parent) {
            target_dir = vfs_ctx.current_dir->parent;
        } else {
            target_dir = vfs_ctx.current_dir; // Already at root
        }
    } else if (vfs_strcmp(path, ".") == 0) {
        // Stay in current directory
        target_dir = vfs_ctx.current_dir;
    } else {
        // Find child directory
        target_dir = vfs_find_child(vfs_ctx.current_dir, path);
        if (!target_dir || target_dir->type != VFS_DIRECTORY) {
            return NULL; // Directory not found
        }
    }
    
    if (target_dir) {
        vfs_ctx.current_dir = target_dir;
        
        // Update current path
        if (target_dir == vfs_ctx.root_dir) {
            vfs_strcpy(vfs_ctx.current_path, "/");
        } else {
            // Build path by traversing up to root
            char temp_path[VFS_MAX_PATH_LEN];
            temp_path[0] = '\0';
            
            vfs_node_t* node = target_dir;
            while (node && node != vfs_ctx.root_dir) {
                char new_temp[VFS_MAX_PATH_LEN];
                vfs_strcpy(new_temp, "/");
                vfs_strcat(new_temp, node->name);
                vfs_strcat(new_temp, temp_path);
                vfs_strcpy(temp_path, new_temp);
                node = node->parent;
            }
            
            if (temp_path[0] == '\0') {
                vfs_strcpy(vfs_ctx.current_path, "/");
            } else {
                vfs_strcpy(vfs_ctx.current_path, temp_path);
            }
        }
    }
    
    return target_dir;
}

int vfs_rmdir(const char* name) {
    if (!name || !vfs_is_valid_name(name)) {
        return -1;
    }
    
    vfs_node_t* target = vfs_find_child(vfs_ctx.current_dir, name);
    if (!target || target->type != VFS_DIRECTORY) {
        return -1; // Not found or not a directory
    }
    
    // Check if directory is empty
    if (target->child_count > 0) {
        return -2; // Directory not empty
    }
    
    // Remove from parent's children list
    for (size_t i = 0; i < vfs_ctx.current_dir->child_count; i++) {
        if (vfs_ctx.current_dir->children[i] == target) {
            // Shift remaining children
            for (size_t j = i; j < vfs_ctx.current_dir->child_count - 1; j++) {
                vfs_ctx.current_dir->children[j] = vfs_ctx.current_dir->children[j + 1];
            }
            vfs_ctx.current_dir->children[vfs_ctx.current_dir->child_count - 1] = NULL;
            vfs_ctx.current_dir->child_count--;
            break;
        }
    }
    
    // Free the directory
    kfree(target);
    return 0;
}

vfs_node_t* vfs_create_file(const char* name, const uint8_t* data, size_t size) {
    if (!name || !vfs_is_valid_name(name) || size > VFS_MAX_FILE_SIZE) {
        return NULL;
    }
    
    // Check if file already exists
    if (vfs_find_child(vfs_ctx.current_dir, name)) {
        return NULL; // Already exists
    }
    
    // Check if we have space
    if (vfs_ctx.current_dir->child_count >= VFS_MAX_FILES) {
        return NULL; // Directory full
    }
    
    // Create new file
    vfs_node_t* new_file = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!new_file) {
        return NULL;
    }
    
    vfs_strcpy(new_file->name, name);
    new_file->type = VFS_FILE;
    new_file->size = size;
    new_file->parent = vfs_ctx.current_dir;
    new_file->child_count = 0;
    new_file->creation_time = 0;
    
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        new_file->children[i] = NULL;
    }
    
    // Allocate and copy data if provided
    if (data && size > 0) {
        new_file->data = (uint8_t*)kmalloc(size);
        if (!new_file->data) {
            kfree(new_file);
            return NULL;
        }
        
        for (size_t i = 0; i < size; i++) {
            new_file->data[i] = data[i];
        }
    } else {
        new_file->data = NULL;
    }
    
    // Add to parent directory
    vfs_ctx.current_dir->children[vfs_ctx.current_dir->child_count++] = new_file;
    
    return new_file;
}

int vfs_delete_file(const char* name) {
    if (!name || !vfs_is_valid_name(name)) {
        return -1;
    }
    
    vfs_node_t* target = vfs_find_child(vfs_ctx.current_dir, name);
    if (!target || target->type != VFS_FILE) {
        return -1; // Not found or not a file
    }
    
    // Remove from parent's children list
    for (size_t i = 0; i < vfs_ctx.current_dir->child_count; i++) {
        if (vfs_ctx.current_dir->children[i] == target) {
            // Shift remaining children
            for (size_t j = i; j < vfs_ctx.current_dir->child_count - 1; j++) {
                vfs_ctx.current_dir->children[j] = vfs_ctx.current_dir->children[j + 1];
            }
            vfs_ctx.current_dir->children[vfs_ctx.current_dir->child_count - 1] = NULL;
            vfs_ctx.current_dir->child_count--;
            break;
        }
    }
    
    // Free file data and node
    if (target->data) {
        kfree(target->data);
    }
    kfree(target);
    return 0;
}

vfs_node_t* vfs_open_file(const char* name) {
    return vfs_find_child(vfs_ctx.current_dir, name);
}

void vfs_get_current_path(char* buffer, size_t buffer_size) {
    if (buffer && buffer_size > 0) {
        size_t path_len = strlen(vfs_ctx.current_path);
        if (path_len < buffer_size) {
            vfs_strcpy(buffer, vfs_ctx.current_path);
        } else {
            buffer[0] = '\0';
        }
    }
}

vfs_node_t* vfs_get_current_dir(void) {
    return vfs_ctx.current_dir;
}

vfs_node_t* vfs_get_root_dir(void) {
    return vfs_ctx.root_dir;
}

void vfs_list_directory(vfs_node_t* dir) {
    if (!dir || dir->type != VFS_DIRECTORY) {
        terminal_writestring("Invalid directory\n");
        return;
    }
    
    if (dir->child_count == 0) {
        terminal_writestring("(empty)\n");
        return;
    }
    
    for (size_t i = 0; i < dir->child_count; i++) {
        vfs_node_t* child = dir->children[i];
        if (child) {
            if (child->type == VFS_DIRECTORY) {
                terminal_writestring("[DIR]  ");
            } else {
                terminal_writestring("[FILE] ");
            }
            terminal_writestring(child->name);
            
            if (child->type == VFS_FILE) {
                terminal_writestring(" (");
                // Simple size display
                if (child->size < 1024) {
                    char size_str[16];
                    int size = child->size;
                    int pos = 0;
                    if (size == 0) {
                        size_str[pos++] = '0';
                    } else {
                        while (size > 0) {
                            size_str[pos++] = '0' + (size % 10);
                            size /= 10;
                        }
                    }
                    // Reverse
                    for (int j = 0; j < pos / 2; j++) {
                        char temp = size_str[j];
                        size_str[j] = size_str[pos - 1 - j];
                        size_str[pos - 1 - j] = temp;
                    }
                    size_str[pos] = '\0';
                    terminal_writestring(size_str);
                    terminal_writestring(" bytes");
                } else {
                    terminal_writestring("large");
                }
                terminal_writestring(")");
            }
            terminal_writestring("\n");
        }
    }
}

int vfs_is_valid_name(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= VFS_MAX_NAME_LEN) {
        return 0;
    }
    
    // Check for invalid characters
    for (size_t i = 0; name[i]; i++) {
        char c = name[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || 
            c == '"' || c == '<' || c == '>' || c == '|' || c < 32) {
            return 0;
        }
    }
    
    // Check for reserved names
    if (vfs_strcmp(name, ".") == 0 || vfs_strcmp(name, "..") == 0) {
        return 0;
    }
    
    return 1;
}