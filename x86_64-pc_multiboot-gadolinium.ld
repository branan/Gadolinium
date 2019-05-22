ENTRY (loader)

/* These must be kept in sync with the kmemlayout.h file */
kernel_VMA = 0xffffffff80000000;
kernel_LMA = 0x00100000;

SECTIONS
{
    . = kernel_LMA;
    .nomap.reclaim :
    {
        *(.multiboot)
        . = ALIGN(16);
        *(.tables.gdt)
        *(.tables.idt)
        . = ALIGN(0x1000);
        *(.tables.paging)
        pml4 = _pml4_arch;
        pidt = _pidt_arch;
        pgdt = _pgdt_arch;
    }

    . = ALIGN(0x1000);
    .nomap.storage :
    {
        bootheap = .;
        . += 0x8000;

        . += 0x2000;
        bootstack_top = .;
    }

    . = ALIGN(0x1000);
    . += kernel_VMA;
    .text : AT(ADDR(.text) - kernel_VMA)
    {
        *(.text)
        *(.text._*)
        *(.gnu.linkonce.t*)
        . = ALIGN(0x1000);
    }

    .rodata : AT(ADDR(.rodata) - kernel_VMA)
    {
        start_ctors = .;
        *(.ctor*)
        end_ctors = .;

        start_dtors = .;
        *(.dtor*)
        end_dtors = .;

        *(.rodata*)
        *(.gnu.linkonce.r*)
        . = ALIGN(0x1000);
    }

    . = ALIGN(0x1000);
    .data : AT(ADDR(.data) - kernel_VMA)
    {
        *(.data)
        *(.gnu.linkonce.d*)
        . = ALIGN(0x1000);
    }

    . = ALIGN(0x1000);
    .bss : AT(ADDR(.bss) - kernel_VMA)
    {
        *(COMMON)
        *(.bss)
        *(.gnu.linkonce.b*)
        . = ALIGN(0x1000);
    }
}
