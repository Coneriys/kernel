#include "../include/elf.h"
#include "../include/memory.h"
#include <stddef.h>

extern void terminal_writestring(const char* data);
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static void* memset(void* s, int c, size_t n) {
    char* p = (char*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = c;
    }
    return s;
}

int elf_validate(void* elf_data) {
    if (!elf_data) {
        terminal_writestring("ELF: NULL data pointer\n");
        return 0;
    }
    
    elf32_ehdr_t* header = (elf32_ehdr_t*)elf_data;
    
    if (header->e_ident[0] != ELFMAG0 ||
        header->e_ident[1] != ELFMAG1 ||
        header->e_ident[2] != ELFMAG2 ||
        header->e_ident[3] != ELFMAG3) {
        terminal_writestring("ELF: Invalid magic number\n");
        return 0;
    }
    
    if (header->e_ident[4] != ELFCLASS32) {
        terminal_writestring("ELF: Not 32-bit format\n");
        return 0;
    }
    
    if (header->e_ident[5] != ELFDATA2LSB) {
        terminal_writestring("ELF: Not little-endian\n");
        return 0;
    }
    
    if (header->e_type != ET_EXEC) {
        terminal_writestring("ELF: Not executable file\n");
        return 0;
    }
    
    if (header->e_machine != EM_386) {
        terminal_writestring("ELF: Not i386 architecture\n");
        return 0;
    }
    
    terminal_writestring("ELF: File validation successful\n");
    return 1;
}

int elf_load(void* elf_data, size_t size) {
    if (!elf_validate(elf_data)) {
        return 0;
    }
    
    elf32_ehdr_t* header = (elf32_ehdr_t*)elf_data;
    elf32_phdr_t* ph_table = (elf32_phdr_t*)((uintptr_t)elf_data + header->e_phoff);
    
    terminal_writestring("ELF: Loading program segments...\n");
    
    for (int i = 0; i < header->e_phnum; i++) {
        elf32_phdr_t* ph = &ph_table[i];
        
        if (ph->p_type != PT_LOAD) {
            continue;
        }
        
        if (ph->p_offset + ph->p_filesz > size) {
            terminal_writestring("ELF: Segment extends beyond file\n");
            return 0;
        }
        
        void* segment_dest = (void*)ph->p_vaddr;
        
        memset(segment_dest, 0, ph->p_memsz);
        
        if (ph->p_filesz > 0) {
            void* segment_src = (void*)((uintptr_t)elf_data + ph->p_offset);
            memcpy(segment_dest, segment_src, ph->p_filesz);
        }
        
        terminal_writestring("ELF: Loaded segment to memory\n");
    }
    
    terminal_writestring("ELF: Program loaded successfully\n");
    return 1;
}

void* elf_get_entry_point(void* elf_data) {
    if (!elf_validate(elf_data)) {
        return NULL;
    }
    
    elf32_ehdr_t* header = (elf32_ehdr_t*)elf_data;
    return (void*)header->e_entry;
}

int elf_execute(void* elf_data, size_t size) {
    terminal_writestring("ELF: Starting program execution...\n");
    
    if (!elf_load(elf_data, size)) {
        terminal_writestring("ELF: Failed to load program\n");
        return -1;
    }
    
    void* entry_point = elf_get_entry_point(elf_data);
    if (!entry_point) {
        terminal_writestring("ELF: Invalid entry point\n");
        return -1;
    }
    
    terminal_writestring("ELF: Jumping to entry point...\n");
    
    elf_entry_point_t program = (elf_entry_point_t)entry_point;
    int result = program();
    
    terminal_writestring("ELF: Program returned\n");
    return result;
}