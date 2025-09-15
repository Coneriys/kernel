#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS32 1
#define ELFDATA2LSB 1

#define ET_EXEC 2

#define EM_386 3

#define PT_LOAD 1

#define PF_X 1
#define PF_W 2
#define PF_R 4

typedef int (*elf_entry_point_t)(void);

int elf_validate(void* elf_data);
int elf_load(void* elf_data, size_t size);
void* elf_get_entry_point(void* elf_data);
int elf_execute(void* elf_data, size_t size);

#endif