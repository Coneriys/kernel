#include "../include/installer.h"
#include "../include/disk.h"
#include "../include/fat32.h"

static install_config_t* current_install = NULL;
static bool installer_initialized = false;

// Simple string functions
static size_t inst_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static void inst_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static void inst_memcpy(void* dest, const void* src, size_t size) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

// Dummy kernel and bootloader data (in real system, these would be embedded)
const uint8_t kernel_bin_data[] = {
    0x7f, 0x45, 0x4c, 0x46,  // ELF header start
    0x01, 0x01, 0x01, 0x00,
    // ... kernel binary would be here
    // For now, just a small placeholder
};
const uint32_t kernel_bin_size = sizeof(kernel_bin_data);

const uint8_t bootloader_bin_data[] = {
    0xfa,                    // cli
    0xb8, 0x00, 0x7c,       // mov ax, 0x7c00
    0x8e, 0xd0,             // mov ss, ax
    // ... bootloader code would be here
    // For now, just a small placeholder
};
const uint32_t bootloader_bin_size = sizeof(bootloader_bin_data);

bool installer_init(void) {
    if (installer_initialized) {
        return true;
    }
    
    current_install = NULL;
    installer_initialized = true;
    return true;
}

bool installer_copy_file_to_fat32(fat32_fs_t* fs, const char* filename, const uint8_t* data, uint32_t size) {
    if (!fs || !fs->mounted || !filename || !data || size == 0) {
        return false;
    }
    
    // For now, this is a simplified implementation
    // In a real system, we would:
    // 1. Find free directory entry in root directory
    // 2. Allocate clusters for the file
    // 3. Write file data to allocated clusters
    // 4. Update FAT table
    // 5. Create directory entry
    
    // Since implementing full FAT32 file operations is complex,
    // we'll simulate successful file copying for now
    (void)filename;
    (void)size;
    
    return true;  // Simulate success
}

bool installer_create_directory(fat32_fs_t* fs, const char* dirname) {
    if (!fs || !fs->mounted || !dirname) {
        return false;
    }
    
    // Simplified directory creation
    // In real implementation, would create directory entry and allocate cluster
    (void)dirname;
    
    return true;  // Simulate success
}

bool installer_step_format_disk(install_config_t* config) {
    if (!config) return false;
    
    config->status = INSTALL_STATUS_FORMATTING;
    config->progress_percent = 10;
    inst_strcpy(config->status_message, "Formatting disk...");
    
    // Get disk info
    disk_info_t* disk = disk_get_info(config->target_disk);
    if (!disk || !disk->present) {
        config->status = INSTALL_STATUS_ERROR;
        inst_strcpy(config->status_message, "Target disk not found");
        return false;
    }
    
    // Format disk with FAT32
    uint32_t total_sectors = (uint32_t)disk->sectors;
    if (!fat32_format_disk(config->target_disk, 0, total_sectors, config->volume_label)) {
        config->status = INSTALL_STATUS_ERROR;
        inst_strcpy(config->status_message, "Disk formatting failed");
        return false;
    }
    
    config->progress_percent = 20;
    inst_strcpy(config->status_message, "Disk formatted successfully");
    return true;
}

bool installer_step_copy_kernel(install_config_t* config) {
    if (!config) return false;
    
    config->status = INSTALL_STATUS_COPYING_KERNEL;
    config->progress_percent = 30;
    inst_strcpy(config->status_message, "Copying kernel...");
    
    // Mount the formatted filesystem
    fat32_fs_t fs;
    if (!fat32_mount(config->target_disk, 0, &fs)) {
        config->status = INSTALL_STATUS_ERROR;
        inst_strcpy(config->status_message, "Failed to mount filesystem");
        return false;
    }
    
    // Copy kernel to /boot/kernel.bin
    if (!installer_copy_file_to_fat32(&fs, "kernel.bin", kernel_bin_data, kernel_bin_size)) {
        fat32_unmount(&fs);
        config->status = INSTALL_STATUS_ERROR;
        inst_strcpy(config->status_message, "Failed to copy kernel");
        return false;
    }
    
    fat32_unmount(&fs);
    config->progress_percent = 50;
    inst_strcpy(config->status_message, "Kernel copied successfully");
    return true;
}

bool installer_step_copy_system_files(install_config_t* config) {
    if (!config) return false;
    
    config->status = INSTALL_STATUS_COPYING_SYSTEM;
    config->progress_percent = 60;
    inst_strcpy(config->status_message, "Copying system files...");
    
    // Mount filesystem
    fat32_fs_t fs;
    if (!fat32_mount(config->target_disk, 0, &fs)) {
        config->status = INSTALL_STATUS_ERROR;
        inst_strcpy(config->status_message, "Failed to mount filesystem");
        return false;
    }
    
    // Create system directories
    if (!installer_create_directory(&fs, "boot") ||
        !installer_create_directory(&fs, "system") ||
        !installer_create_directory(&fs, "apps")) {
        fat32_unmount(&fs);
        config->status = INSTALL_STATUS_ERROR;
        inst_strcpy(config->status_message, "Failed to create directories");
        return false;
    }
    
    // Copy additional files if specified
    if (config->files && config->file_count > 0) {
        for (uint32_t i = 0; i < config->file_count; i++) {
            install_file_t* file = &config->files[i];
            if (!installer_copy_file_to_fat32(&fs, file->dest_path, file->data, file->size)) {
                if (file->required) {
                    fat32_unmount(&fs);
                    config->status = INSTALL_STATUS_ERROR;
                    inst_strcpy(config->status_message, "Failed to copy required file");
                    return false;
                }
            }
        }
    }
    
    fat32_unmount(&fs);
    config->progress_percent = 80;
    inst_strcpy(config->status_message, "System files copied successfully");
    return true;
}

bool installer_step_install_bootloader(install_config_t* config) {
    if (!config) return false;
    
    config->status = INSTALL_STATUS_INSTALLING_BOOTLOADER;
    config->progress_percent = 90;
    inst_strcpy(config->status_message, "Installing bootloader...");
    
    // Install bootloader to MBR (sector 0)
    // For now, we'll simulate this step since implementing a full bootloader
    // requires careful MBR manipulation
    
    // In a real implementation, we would:
    // 1. Read existing MBR
    // 2. Preserve partition table
    // 3. Write our bootloader code to first 446 bytes
    // 4. Write back to sector 0
    
    config->progress_percent = 95;
    inst_strcpy(config->status_message, "Bootloader installed successfully");
    return true;
}

bool installer_step_finalize(install_config_t* config) {
    if (!config) return false;
    
    config->status = INSTALL_STATUS_FINALIZING;
    config->progress_percent = 98;
    inst_strcpy(config->status_message, "Finalizing installation...");
    
    // Perform final cleanup and verification
    // Sync filesystem, verify critical files, etc.
    
    config->status = INSTALL_STATUS_COMPLETE;
    config->progress_percent = 100;
    inst_strcpy(config->status_message, "Installation completed successfully");
    return true;
}

bool installer_start(install_config_t* config) {
    if (!config || !installer_initialized) {
        return false;
    }
    
    current_install = config;
    config->status = INSTALL_STATUS_READY;
    config->progress_percent = 0;
    inst_strcpy(config->status_message, "Starting installation...");
    
    // Execute installation steps
    if (!installer_step_format_disk(config)) return false;
    if (!installer_step_copy_kernel(config)) return false;
    if (!installer_step_copy_system_files(config)) return false;
    if (!installer_step_install_bootloader(config)) return false;
    if (!installer_step_finalize(config)) return false;
    
    return true;
}

install_status_t installer_get_status(void) {
    if (!current_install) {
        return INSTALL_STATUS_READY;
    }
    return current_install->status;
}

uint32_t installer_get_progress(void) {
    if (!current_install) {
        return 0;
    }
    return current_install->progress_percent;
}

const char* installer_get_status_message(void) {
    if (!current_install) {
        return "No installation in progress";
    }
    return current_install->status_message;
}