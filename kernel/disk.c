#include "../include/disk.h"
#include "../include/interrupts.h"

static disk_info_t disks[MAX_DISKS];
static bool disk_system_initialized = false;

// I/O port operations
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static void ata_wait_bsy(uint16_t base) {
    while (inb(base + ATA_REG_STATUS) & ATA_STATUS_BSY) {
        // Wait for BSY to clear
    }
}

static void ata_wait_drq(uint16_t base) {
    while (!(inb(base + ATA_REG_STATUS) & ATA_STATUS_DRQ)) {
        // Wait for DRQ to set
    }
}

static bool ata_identify(uint16_t base, uint8_t drive, uint16_t* buffer) {
    // Select drive
    outb(base + ATA_REG_DRIVE, 0xA0 | (drive << 4));
    
    // Wait for drive to be ready
    ata_wait_bsy(base);
    
    // Send IDENTIFY command
    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    // Check if drive exists
    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status == 0) {
        return false; // Drive doesn't exist
    }
    
    // Wait for BSY to clear
    ata_wait_bsy(base);
    
    // Check for errors
    if (inb(base + ATA_REG_STATUS) & ATA_STATUS_ERR) {
        return false;
    }
    
    // Wait for data to be ready
    ata_wait_drq(base);
    
    // Read 256 words (512 bytes) of identification data
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base + ATA_REG_DATA);
    }
    
    return true;
}

static bool ata_read_sectors(uint16_t base, uint8_t drive, uint32_t lba, uint8_t count, void* buffer) {
    uint16_t* buf = (uint16_t*)buffer;
    
    // Select drive and set LBA mode
    outb(base + ATA_REG_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    
    // Wait for drive to be ready
    ata_wait_bsy(base);
    
    // Set sector count and LBA
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA_LO, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    // Send READ SECTORS command
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
    
    // Read each sector
    for (int sector = 0; sector < count; sector++) {
        // Wait for data to be ready
        ata_wait_drq(base);
        
        // Check for errors
        if (inb(base + ATA_REG_STATUS) & ATA_STATUS_ERR) {
            return false;
        }
        
        // Read 256 words (512 bytes) per sector
        for (int i = 0; i < 256; i++) {
            buf[sector * 256 + i] = inw(base + ATA_REG_DATA);
        }
    }
    
    return true;
}

static bool ata_write_sectors(uint16_t base, uint8_t drive, uint32_t lba, uint8_t count, const void* buffer) {
    const uint16_t* buf = (const uint16_t*)buffer;
    
    // Select drive and set LBA mode
    outb(base + ATA_REG_DRIVE, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    
    // Wait for drive to be ready
    ata_wait_bsy(base);
    
    // Set sector count and LBA
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA_LO, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    // Send WRITE SECTORS command
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    // Write each sector
    for (int sector = 0; sector < count; sector++) {
        // Wait for drive to be ready for data
        ata_wait_drq(base);
        
        // Check for errors
        if (inb(base + ATA_REG_STATUS) & ATA_STATUS_ERR) {
            return false;
        }
        
        // Write 256 words (512 bytes) per sector
        for (int i = 0; i < 256; i++) {
            outw(base + ATA_REG_DATA, buf[sector * 256 + i]);
        }
    }
    
    return true;
}

bool disk_detect(uint32_t disk_id) {
    if (disk_id >= MAX_DISKS) {
        return false;
    }
    
    uint16_t identify_buffer[256];
    uint16_t base;
    uint8_t drive;
    
    // Determine base address and drive number
    if (disk_id < 2) {
        base = ATA_PRIMARY_BASE;
        drive = disk_id;
    } else {
        base = ATA_SECONDARY_BASE;
        drive = disk_id - 2;
    }
    
    // Try to identify the drive
    if (!ata_identify(base, drive, identify_buffer)) {
        disks[disk_id].present = false;
        return false;
    }
    
    // Parse identification data
    disk_info_t* disk = &disks[disk_id];
    disk->disk_id = disk_id;
    disk->present = true;
    disk->type = DISK_TYPE_ATA_PATA;
    disk->sector_size = DISK_SECTOR_SIZE;
    
    // Get drive capacity
    if (identify_buffer[83] & (1 << 10)) { // LBA48 support
        disk->is_lba48 = true;
        disk->sectors = *((uint64_t*)&identify_buffer[100]);
    } else {
        disk->is_lba48 = false;
        disk->sectors = *((uint32_t*)&identify_buffer[60]);
    }
    
    // Extract model string (words 27-46)
    for (int i = 0; i < 20; i++) {
        disk->model[i * 2] = (identify_buffer[27 + i] >> 8) & 0xFF;
        disk->model[i * 2 + 1] = identify_buffer[27 + i] & 0xFF;
    }
    disk->model[40] = '\0';
    
    // Extract serial number (words 10-19)
    for (int i = 0; i < 10; i++) {
        disk->serial[i * 2] = (identify_buffer[10 + i] >> 8) & 0xFF;
        disk->serial[i * 2 + 1] = identify_buffer[10 + i] & 0xFF;
    }
    disk->serial[20] = '\0';
    
    return true;
}

bool disk_init(void) {
    if (disk_system_initialized) {
        return true;
    }
    
    // Initialize disk info structures
    for (int i = 0; i < MAX_DISKS; i++) {
        disks[i].disk_id = i;
        disks[i].present = false;
        disks[i].type = DISK_TYPE_UNKNOWN;
    }
    
    // Detect all possible drives
    int detected_count = 0;
    for (int i = 0; i < MAX_DISKS; i++) {
        if (disk_detect(i)) {
            detected_count++;
        }
    }
    
    disk_system_initialized = true;
    return detected_count > 0;
}

disk_info_t* disk_get_info(uint32_t disk_id) {
    if (disk_id >= MAX_DISKS || !disks[disk_id].present) {
        return NULL;
    }
    return &disks[disk_id];
}

bool disk_read_sectors(uint32_t disk_id, uint64_t lba, uint32_t count, void* buffer) {
    if (disk_id >= MAX_DISKS || !disks[disk_id].present) {
        return false;
    }
    
    if (lba + count > disks[disk_id].sectors) {
        return false; // Out of bounds
    }
    
    uint16_t base;
    uint8_t drive;
    
    // Determine base address and drive number
    if (disk_id < 2) {
        base = ATA_PRIMARY_BASE;
        drive = disk_id;
    } else {
        base = ATA_SECONDARY_BASE;
        drive = disk_id - 2;
    }
    
    return ata_read_sectors(base, drive, (uint32_t)lba, (uint8_t)count, buffer);
}

bool disk_write_sectors(uint32_t disk_id, uint64_t lba, uint32_t count, const void* buffer) {
    if (disk_id >= MAX_DISKS || !disks[disk_id].present) {
        return false;
    }
    
    if (lba + count > disks[disk_id].sectors) {
        return false; // Out of bounds
    }
    
    uint16_t base;
    uint8_t drive;
    
    // Determine base address and drive number
    if (disk_id < 2) {
        base = ATA_PRIMARY_BASE;
        drive = disk_id;
    } else {
        base = ATA_SECONDARY_BASE;
        drive = disk_id - 2;
    }
    
    return ata_write_sectors(base, drive, (uint32_t)lba, (uint8_t)count, buffer);
}

void disk_list_devices(void) {
    for (int i = 0; i < MAX_DISKS; i++) {
        if (disks[i].present) {
            // This would print disk info in a real system
            // For now, just mark that the disk exists
        }
    }
}