#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_MAX_NAME_LEN 64
#define VFS_MAX_FILES 256
#define VFS_MAX_FILE_SIZE 4096
#define VFS_MAX_PATH_LEN 512

typedef enum {
    VFS_FILE,
    VFS_DIRECTORY
} vfs_type_t;

typedef struct vfs_node {
    char name[VFS_MAX_NAME_LEN];
    vfs_type_t type;
    size_t size;
    uint8_t* data;
    struct vfs_node* parent;
    struct vfs_node* children[VFS_MAX_FILES];
    size_t child_count;
    uint32_t creation_time;
} vfs_node_t;

typedef struct {
    vfs_node_t* current_dir;
    vfs_node_t* root_dir;
    char current_path[VFS_MAX_PATH_LEN];
} vfs_context_t;

// VFS initialization
void vfs_init(void);

// Directory operations
vfs_node_t* vfs_mkdir(const char* name);
vfs_node_t* vfs_chdir(const char* path);
int vfs_rmdir(const char* name);
vfs_node_t* vfs_find_child(vfs_node_t* parent, const char* name);

// File operations
vfs_node_t* vfs_create_file(const char* name, const uint8_t* data, size_t size);
int vfs_delete_file(const char* name);
vfs_node_t* vfs_open_file(const char* name);

// Path operations
void vfs_get_current_path(char* buffer, size_t buffer_size);
vfs_node_t* vfs_get_current_dir(void);
vfs_node_t* vfs_get_root_dir(void);

// Utility functions
void vfs_list_directory(vfs_node_t* dir);
int vfs_is_valid_name(const char* name);

#endif