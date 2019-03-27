.global _pml4_arch
.global _pgdt_arch
.global _pidt_arch

.section .tables.paging, "a", @progbits

# These tables identity map the low 4GB, as well as shadowing the
# low 2GB as the top 2GB. They also map the page table to itself
# as the first chunk of memory in the high half and set up the initial
# heap and stack from the memory reserved in the kernel's BSS section
#

# What this really means is that we need to be loadable at a
# predictable address below 2GB. Any platform/bootloader that
# doesn't support that can't use these tables, and will need to
# discard this section in their linker script and provide their
# own

_pml4_arch:
    .quad (low_pdp + 0x0000000000000003)
    .fill 255, 8, 0
    .quad (pml4 + 0x0000000000000003)
    .quad 0x0000000000000000 # secondary page mapping for forking. Empty for now.
    .quad (heap_pdp + 0x0000000000000003)
    .fill 252, 8, 0
    .quad (high_pdp + 0x0000000000000003)

heap_pdp:
    .quad (heap_pd + 0x0000000000000003)
    .fill 511, 8, 0

heap_pd:
    .quad (heap_pt + 0x0000000000000003)
    .fill 511, 8, 0

heap_pt:
    i = bootheap
    .rept 8
    .quad (i + 0x0000000000000003)
    i = i + 0x1000
    .endr
    .fill 504, 8, 0

high_pdp:
    .fill 509, 8, 0
    .quad (stack_pd + 0x0000000000000003)
    .quad (low_pd + 0x0000000000000003)
    .quad (low_pd + 0x0000000000001003)

stack_pd:
    .fill 511, 8, 0
    .quad (stack_pt + 0x0000000000000003)

stack_pt:
    .fill 510, 8, 0
    .quad (bootstack_top - 0x0000000000002000 + 3)
    .quad (bootstack_top - 0x0000000000001000 + 3)

low_pdp:
    .quad (low_pd + 0x0000000000000003)
    .quad (low_pd + 0x0000000000001003)
    .quad (low_pd + 0x0000000000002003)
    .quad (low_pd + 0x0000000000003003)
    .fill 508, 8, 0

low_pd:
    i = 0
    .rept 2048
    .quad ( i + 0x0000000000000083)
    i = i + 0x200000
    .endr

.section .tables.gdt, "a", @progbits
# This is a small GDT for bootup. We'll set up the full one later
GDT_START:
    .quad   0x0000000000000000      # NULL descriptor
    .quad   0x00af9a000000ffff      # Kernel CS
    .quad   0x00cf92000000ffff      # Kernel DS
GDT_END:

    .word 0 # padding

_pgdt_arch:
    .word   GDT_END - GDT_START - 1
    .quad   GDT_START

    .word 0 #padding

.section .tables.idt, "a", @progbits
_pidt_arch:
    .word 0
    .quad 0
