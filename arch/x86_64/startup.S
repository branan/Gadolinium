#define ASM_FILE
#include "kmemlayout.h"

.global start64
.global start32

.code32

# All functions in this file MUST preserve RDI, RSI, RDX, and
# RCX. These are all potential arguments from platform-specific
# bootloader code that will be passed to the platform init function

.set start32, _start32-KERNEL_VMA_BASE
_start32:
    cli
    # RCX is a potential argument from platform startup code to
    # kinit. We need to preserve it. (we need it for MSR access)
    movl %ecx, %ebx

    # enable 64-bit page table with CR4.PAE
    movl %cr4, %eax
    bts $5, %eax
    movl %eax, %cr4

    # Load our initial paging structures
    movl $pml4, %eax
    movl %eax, %cr3

    # enable long mode
    movl $0xc0000080, %ecx
    rdmsr
    bts $8, %eax
    wrmsr

    lgdt (pgdt)
    lidt (pidt)

    movl %cr0, %eax
    bts $31, %eax
    movl %eax, %cr0

    # restore ECX, then jump into long mode
    movl %ebx, %ecx
    ljmp $KERNEL_CODE_SEGMENT, $kboot_low

.code64

.set start64, _start64-KERNEL_VMA_BASE
_start64:
    cli
    # If we're coming in already in long mode, we just need to set up our GDT and IDT,
    # and point to our paging structures.
    movq $(pml4), %rax
    movq %rax, %cr3
    lgdt (pgdt)
    lidt (pidt)
    # fall through to stack setup and leap into high memory mapping

.set kboot_low, _kboot_low-KERNEL_VMA_BASE
_kboot_low:
    movq $KERNEL_STACK_TOP, %rsp
    pushq $KERNEL_CODE_SEGMENT
    pushq $kboot
    lretq

kboot:
    movq $KERNEL_STACK_TOP, %rsp
    mov $KERNEL_DATA_SEGMENT, %ax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Any arguments for kinit will have come from code that calls us,
    # and have been preserved by functions in this file.
    call kinit
    test %eax, %eax # kinit returns false if it fails. It handles earlyprint-ing a message
    jz halt

    mov %rax, %rdi
#    call kmain
halt:
   hlt
