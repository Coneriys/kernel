#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define DISK_SECTOR_SIZE 512
#define MAX_DISKS 4

typedef enum {
    DISK_TYPE_UNKNOWN = 0,
    DISK_TYPE_ATA_PATA,
    DISK_TYPE_ATA_SATA,
    DISK_TYPE_ATAPI
} disk_type_t;

typedef struct {
    uint32_t disk_id;
    disk_type_t type;
    bool present;
    uint64_t sectors;
    uint32_t sector_size;
    char model[41];
    char serial[21];
    bool is_lba48;
} disk_info_t;

// ATA registers
#define ATA_PRIMARY_BASE   0x1F0
#define ATA_SECONDARY_BASE 0x170

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT   0x02
#define ATA_REG_LBA_LO     0x03
#define ATA_REG_LBA_MID    0x04
#define ATA_REG_LBA_HI     0x05
#define ATA_REG_DRIVE      0x06
#define ATA_REG_STATUS     0x07
#define ATA_REG_COMMAND    0x07

// ATA commands
#define ATA_CMD_READ_SECTORS     0x20
#define ATA_CMD_WRITE_SECTORS    0x30
#define ATA_CMD_IDENTIFY         0xEC
#define ATA_CMD_READ_SECTORS_EXT 0x24
#define ATA_CMD_WRITE_SECTORS_EXT 0x34

// ATA status bits
#define ATA_STATUS_ERR  0x01
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_SRV  0x10
#define ATA_STATUS_DF   0x20
#define ATA_STATUS_RDY  0x40
#define ATA_STATUS_BSY  0x80

// Function prototypes
bool disk_init(void);
bool disk_detect(uint32_t disk_id);
disk_info_t* disk_get_info(uint32_t disk_id);
bool disk_read_sectors(uint32_t disk_id, uint64_t lba, uint32_t count, void* buffer);
bool disk_write_sectors(uint32_t disk_id, uint64_t lba, uint32_t count, const void* buffer);
void disk_list_devices(void);

// Internal functions (implemented in disk.c)

#endif // DISK_H