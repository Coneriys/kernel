#include "../include/fat32.h"
#include "../include/disk.h"

// Simple string functions
static size_t fat32_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static void fat32_memset(void* ptr, int value, size_t size) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = (unsigned char)value;
    }
}

static void fat32_memcpy(void* dest, const void* src, size_t size) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

uint32_t fat32_calculate_clusters(uint32_t total_sectors, uint32_t reserved_sectors, uint8_t num_fats, uint8_t sectors_per_cluster) {
    // Calculate FAT size needed for the given disk size
    uint32_t data_sectors = total_sectors - reserved_sectors;
    uint32_t cluster_count = data_sectors / sectors_per_cluster;
    
    // Each FAT entry is 4 bytes for FAT32
    uint32_t fat_size_bytes = cluster_count * 4;
    uint32_t fat_size_sectors = (fat_size_bytes + 511) / 512;  // Round up
    
    // Adjust for the space taken by FAT tables themselves
    data_sectors -= fat_size_sectors * num_fats;
    cluster_count = data_sectors / sectors_per_cluster;
    
    return cluster_count;
}

bool fat32_write_boot_sector(uint32_t disk_id, uint32_t sector, fat32_boot_sector_t* boot_sector) {
    uint8_t buffer[512];
    fat32_memset(buffer, 0, 512);
    
    // Copy boot sector structure to buffer
    fat32_memcpy(buffer, boot_sector, sizeof(fat32_boot_sector_t));
    
    // Add boot signature
    buffer[510] = 0x55;
    buffer[511] = 0xAA;
    
    return disk_write_sectors(disk_id, sector, 1, buffer);
}

bool fat32_write_fs_info(uint32_t disk_id, uint32_t sector, fat32_fs_info_t* fs_info) {
    uint8_t buffer[512];
    fat32_memset(buffer, 0, 512);
    
    // Copy FS info structure to buffer
    fat32_memcpy(buffer, fs_info, sizeof(fat32_fs_info_t));
    
    return disk_write_sectors(disk_id, sector, 1, buffer);
}

bool fat32_clear_fat_table(uint32_t disk_id, uint32_t fat_start, uint32_t fat_sectors) {
    uint8_t buffer[512];
    fat32_memset(buffer, 0, 512);
    
    // Clear all FAT sectors
    for (uint32_t i = 0; i < fat_sectors; i++) {
        if (!disk_write_sectors(disk_id, fat_start + i, 1, buffer)) {
            return false;
        }
    }
    
    // Set special values in first FAT sector
    uint32_t fat_buffer[128];  // 512 bytes / 4 bytes per entry
    fat32_memset(fat_buffer, 0, 512);
    
    fat_buffer[0] = 0x0FFFFF00 | 0xF8;  // Media type in low byte
    fat_buffer[1] = 0x0FFFFFFF;          // End of chain
    fat_buffer[2] = 0x0FFFFFFF;          // Root directory cluster (end of chain)
    
    return disk_write_sectors(disk_id, fat_start, 1, fat_buffer);
}

bool fat32_create_root_directory(uint32_t disk_id, uint32_t data_start, const char* volume_label) {
    uint8_t buffer[512];
    fat32_memset(buffer, 0, 512);
    
    // Create volume label entry if provided
    if (volume_label && fat32_strlen(volume_label) > 0) {
        fat32_dir_entry_t* entry = (fat32_dir_entry_t*)buffer;
        
        // Copy volume label (max 11 chars, pad with spaces)
        fat32_memset(entry->name, ' ', 11);
        size_t label_len = fat32_strlen(volume_label);
        if (label_len > 11) label_len = 11;
        fat32_memcpy(entry->name, volume_label, label_len);
        
        entry->attr = FAT32_ATTR_VOLUME_ID;
        entry->first_cluster_lo = 0;
        entry->first_cluster_hi = 0;
        entry->file_size = 0;
    }
    
    return disk_write_sectors(disk_id, data_start, 1, buffer);
}

bool fat32_format_disk(uint32_t disk_id, uint32_t start_sector, uint32_t total_sectors, const char* volume_label) {
    disk_info_t* disk_info = disk_get_info(disk_id);
    if (!disk_info || !disk_info->present) {
        return false;
    }
    
    // Calculate filesystem parameters
    uint8_t sectors_per_cluster = 8;  // 4KB clusters for most sizes
    uint16_t reserved_sectors = 32;   // Standard for FAT32
    uint8_t num_fats = 2;             // Always 2 for redundancy
    
    // Adjust cluster size based on disk size
    if (total_sectors > 1048576) {  // > 512MB
        sectors_per_cluster = 16;   // 8KB clusters
    }
    if (total_sectors > 4194304) {  // > 2GB
        sectors_per_cluster = 32;   // 16KB clusters
    }
    
    uint32_t cluster_count = fat32_calculate_clusters(total_sectors, reserved_sectors, num_fats, sectors_per_cluster);
    
    // Ensure we have enough clusters for FAT32 (minimum 65525)
    if (cluster_count < 65525) {
        return false;  // Too small for FAT32
    }
    
    // Calculate FAT size
    uint32_t fat_size_sectors = (cluster_count * 4 + 511) / 512;
    
    // Create boot sector
    fat32_boot_sector_t boot_sector;
    fat32_memset(&boot_sector, 0, sizeof(boot_sector));
    
    // Boot jump instruction
    boot_sector.jump[0] = 0xEB;
    boot_sector.jump[1] = 0x58;
    boot_sector.jump[2] = 0x90;
    
    // OEM name
    fat32_memcpy(boot_sector.oem_name, "ByteOS  ", 8);
    
    // BPB (BIOS Parameter Block)
    boot_sector.bytes_per_sector = 512;
    boot_sector.sectors_per_cluster = sectors_per_cluster;
    boot_sector.reserved_sectors = reserved_sectors;
    boot_sector.num_fats = num_fats;
    boot_sector.root_entries = 0;  // Must be 0 for FAT32
    boot_sector.total_sectors_16 = 0;  // Must be 0 for FAT32
    boot_sector.media_type = 0xF8;  // Fixed disk
    boot_sector.fat_size_16 = 0;  // Must be 0 for FAT32
    boot_sector.sectors_per_track = 63;
    boot_sector.num_heads = 255;
    boot_sector.hidden_sectors = start_sector;
    boot_sector.total_sectors_32 = total_sectors;
    
    // FAT32 specific fields
    boot_sector.fat_size_32 = fat_size_sectors;
    boot_sector.ext_flags = 0;
    boot_sector.fs_version = 0;
    boot_sector.root_cluster = 2;  // Root directory starts at cluster 2
    boot_sector.fs_info = 1;  // FS Info at sector 1
    boot_sector.backup_boot_sector = 6;  // Backup boot sector at sector 6
    
    boot_sector.drive_number = 0x80;  // Hard disk
    boot_sector.boot_signature = 0x29;
    boot_sector.volume_id = 0x12345678;  // Random volume ID
    
    // Volume label
    fat32_memset(boot_sector.volume_label, ' ', 11);
    if (volume_label) {
        size_t label_len = fat32_strlen(volume_label);
        if (label_len > 11) label_len = 11;
        fat32_memcpy(boot_sector.volume_label, volume_label, label_len);
    } else {
        fat32_memcpy(boot_sector.volume_label, "NO NAME    ", 11);
    }
    
    fat32_memcpy(boot_sector.fs_type, "FAT32   ", 8);
    
    // Write boot sector
    if (!fat32_write_boot_sector(disk_id, start_sector, &boot_sector)) {
        return false;
    }
    
    // Create and write FS Info sector
    fat32_fs_info_t fs_info;
    fat32_memset(&fs_info, 0, sizeof(fs_info));
    
    fs_info.lead_signature = 0x41615252;
    fs_info.struct_signature = 0x61417272;
    fs_info.free_count = cluster_count - 1;  // All clusters free except root
    fs_info.next_free = 3;  // Start searching from cluster 3
    fs_info.trail_signature = 0xAA550000;
    
    if (!fat32_write_fs_info(disk_id, start_sector + 1, &fs_info)) {
        return false;
    }
    
    // Write backup boot sector
    if (!fat32_write_boot_sector(disk_id, start_sector + 6, &boot_sector)) {
        return false;
    }
    
    // Clear and initialize FAT tables
    uint32_t fat_start = start_sector + reserved_sectors;
    for (int fat = 0; fat < num_fats; fat++) {
        if (!fat32_clear_fat_table(disk_id, fat_start + fat * fat_size_sectors, fat_size_sectors)) {
            return false;
        }
    }
    
    // Create root directory
    uint32_t data_start = fat_start + (num_fats * fat_size_sectors);
    if (!fat32_create_root_directory(disk_id, data_start, volume_label)) {
        return false;
    }
    
    return true;
}

bool fat32_mount(uint32_t disk_id, uint32_t start_sector, fat32_fs_t* fs) {
    if (!fs) return false;
    
    // Read boot sector
    uint8_t buffer[512];
    if (!disk_read_sectors(disk_id, start_sector, 1, buffer)) {
        return false;
    }
    
    fat32_memcpy(&fs->boot_sector, buffer, sizeof(fat32_boot_sector_t));
    
    // Verify FAT32 signature
    if (fs->boot_sector.bytes_per_sector != 512 ||
        fs->boot_sector.root_entries != 0 ||
        fs->boot_sector.fat_size_16 != 0 ||
        fs->boot_sector.total_sectors_16 != 0) {
        return false;
    }
    
    // Calculate important sectors
    fs->disk_id = disk_id;
    fs->start_sector = start_sector;
    fs->total_sectors = fs->boot_sector.total_sectors_32;
    fs->fat_start_sector = start_sector + fs->boot_sector.reserved_sectors;
    fs->data_start_sector = fs->fat_start_sector + (fs->boot_sector.num_fats * fs->boot_sector.fat_size_32);
    fs->root_dir_cluster = fs->boot_sector.root_cluster;
    fs->mounted = true;
    
    return true;
}

bool fat32_unmount(fat32_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return false;
    }
    
    fs->mounted = false;
    fat32_memset(fs, 0, sizeof(fat32_fs_t));
    return true;
}