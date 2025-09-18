#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// FAT32 structures
typedef struct __attribute__((packed)) {
    uint8_t  jump[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;  // Must be 0 for FAT32
    uint16_t total_sectors_16;  // Must be 0 for FAT32
    uint8_t  media_type;
    uint16_t fat_size_16;  // Must be 0 for FAT32
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    
    // FAT32 specific
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} fat32_boot_sector_t;

typedef struct __attribute__((packed)) {
    uint32_t lead_signature;     // 0x41615252
    uint8_t  reserved1[480];
    uint32_t struct_signature;   // 0x61417272
    uint32_t free_count;
    uint32_t next_free;
    uint8_t  reserved2[12];
    uint32_t trail_signature;    // 0xAA550000
} fat32_fs_info_t;

typedef struct __attribute__((packed)) {
    char     name[8];
    char     ext[3];
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_lo;
    uint32_t file_size;
} fat32_dir_entry_t;

// File attributes
#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_ATTR_LONG_NAME  0x0F

// Cluster values
#define FAT32_FREE_CLUSTER    0x00000000
#define FAT32_EOC_MARK        0x0FFFFFF8
#define FAT32_BAD_CLUSTER     0x0FFFFFF7

typedef struct {
    uint32_t disk_id;
    uint32_t start_sector;
    uint32_t total_sectors;
    fat32_boot_sector_t boot_sector;
    uint32_t fat_start_sector;
    uint32_t data_start_sector;
    uint32_t root_dir_cluster;
    bool mounted;
} fat32_fs_t;

// Function prototypes
bool fat32_format_disk(uint32_t disk_id, uint32_t start_sector, uint32_t total_sectors, const char* volume_label);
bool fat32_mount(uint32_t disk_id, uint32_t start_sector, fat32_fs_t* fs);
bool fat32_unmount(fat32_fs_t* fs);

// Internal functions
bool fat32_write_boot_sector(uint32_t disk_id, uint32_t sector, fat32_boot_sector_t* boot_sector);
bool fat32_write_fs_info(uint32_t disk_id, uint32_t sector, fat32_fs_info_t* fs_info);
bool fat32_clear_fat_table(uint32_t disk_id, uint32_t fat_start, uint32_t fat_sectors);
bool fat32_create_root_directory(uint32_t disk_id, uint32_t data_start, const char* volume_label);

uint32_t fat32_calculate_clusters(uint32_t total_sectors, uint32_t reserved_sectors, uint8_t num_fats, uint8_t sectors_per_cluster);

#endif // FAT32_H