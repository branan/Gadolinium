#pragma once

// The heap starts just above the recursive mappings.
#define KERNEL_HEAP_START 0xffff810000000000
// The stack eventually starts at the other end of kernel memory space,
// just below the kernel's 2GB code region
#define KERNEL_STACK_TOP 0xffffffff80000000
// So the base virtual memory address of the kernel is at the same place
#define KERNEL_VMA_BASE 0xffffffff80000000
// The LMA base should possible be defined in the platform - it's
// wherever we expect to be loaded by the bootloader.
#define KERNEL_LMA_BASE 0x0000000000100000

#define KERNEL_CODE_SEGMENT 0x08
#define KERNEL_DATA_SEGMENT 0x10
// Unused 32-bit user code segment at 0x18
#define USER_CODE_SEGMENT 0x20
#define USER_DATA_SEGMENT 0x28
#define CORE0_STATE_SEGMENT 0x30

// The two recursive mapping areas. On x86_64, these are the two
// lowest 512GB chunks of the high-half of the virtual memory space.
#define RECURSIVE_MAPPING_INDEX 0x100
#define FORK_MAPPING_INDEX 0x101
#define RECURSIVE_MAPPING 0xFFFF000000000000 | (RECURSIVE_MAPPING_INDEX << 39)
#define FORK_MAPPING 0xFFFF000000000000 | (FORK_MAPPING_INDEX << 39)

#define HUGE_PAGE_SIZE 0x40000000
#define HUGE_PAGE_MASK 0x3fffffff
#define BIG_PAGE_SIZE 0x200000
#define BIG_PAGE_MASK 0x1fffff
#define PAGE_SIZE 0x1000
#define PAGE_MASK 0x0fff

// 16-byte alignment is nice and clean :)
#define ALLOC_ALIGN 0x10
#define ALLOC_MASK 0x0F
