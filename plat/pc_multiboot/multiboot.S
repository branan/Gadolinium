#define ASM_FILE
#include "multiboot2.h"
#include "kmemlayout.h"

.global loader # making entry point visible to linker
.code32 

# setting up the Multiboot header - see GRUB docs for details
.set MBSIZE, (mbhdr_end - mbhdr_start)
.set CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MBSIZE)         # checksum required

.section .multiboot, "a", @progbits

.align MULTIBOOT_HEADER_ALIGN
mbhdr_start:
.long MULTIBOOT2_HEADER_MAGIC
.long MULTIBOOT_ARCHITECTURE_I386
.long MBSIZE
.long CHECKSUM

.align MULTIBOOT_TAG_ALIGN
.short MULTIBOOT_HEADER_TAG_MODULE_ALIGN
.short 0
.long 8

.align MULTIBOOT_TAG_ALIGN
.short MULTIBOOT_HEADER_TAG_END
.short 0
.long 8
mbhdr_end:

loader:
    movl %ebx, %esi
    movl %eax, %edi
    movl $bootstack_top, %esp # the PHYSICAL address of our boot stack region
    push $(start32-KERNEL_VMA_BASE)
    ret
