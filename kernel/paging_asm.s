.section .text

.global load_page_directory
.type load_page_directory, @function
load_page_directory:
    push %ebp
    mov %esp, %ebp
    mov 8(%esp), %eax
    mov %eax, %cr3
    mov %ebp, %esp
    pop %ebp
    ret

.global enable_paging_asm
.type enable_paging_asm, @function
enable_paging_asm:
    push %ebp
    mov %esp, %ebp
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    mov %ebp, %esp
    pop %ebp
    ret

.global get_cr3
.type get_cr3, @function
get_cr3:
    mov %cr3, %eax
    ret

.global get_cr2
.type get_cr2, @function
get_cr2:
    mov %cr2, %eax
    ret