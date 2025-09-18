#ifndef INSTALLER_H
#define INSTALLER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../include/fat32.h"

// Installation status
typedef enum {
    INSTALL_STATUS_READY = 0,
    INSTALL_STATUS_FORMATTING,
    INSTALL_STATUS_COPYING_KERNEL,
    INSTALL_STATUS_COPYING_SYSTEM,
    INSTALL_STATUS_INSTALLING_BOOTLOADER,
    INSTALL_STATUS_FINALIZING,
    INSTALL_STATUS_COMPLETE,
    INSTALL_STATUS_ERROR
} install_status_t;

// File to copy during installation
typedef struct {
    const char* source_name;
    const char* dest_path;
    const uint8_t* data;
    uint32_t size;
    bool required;
} install_file_t;

// Installation configuration
typedef struct {
    uint32_t target_disk;
    const char* volume_label;
    install_file_t* files;
    uint32_t file_count;
    install_status_t status;
    uint32_t progress_percent;
    char status_message[128];
} install_config_t;

// Function prototypes
bool installer_init(void);
bool installer_start(install_config_t* config);
bool installer_copy_file_to_fat32(fat32_fs_t* fs, const char* filename, const uint8_t* data, uint32_t size);
bool installer_create_directory(fat32_fs_t* fs, const char* dirname);
install_status_t installer_get_status(void);
uint32_t installer_get_progress(void);
const char* installer_get_status_message(void);

// Built-in system files
extern const uint8_t kernel_bin_data[];
extern const uint32_t kernel_bin_size;

extern const uint8_t bootloader_bin_data[];
extern const uint32_t bootloader_bin_size;

// Installation steps
bool installer_step_format_disk(install_config_t* config);
bool installer_step_copy_kernel(install_config_t* config);
bool installer_step_copy_system_files(install_config_t* config);
bool installer_step_install_bootloader(install_config_t* config);
bool installer_step_finalize(install_config_t* config);

#endif // INSTALLER_H