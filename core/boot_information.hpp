#pragma once

#include "stddef.h"
#include "elf64.h"
#include "string.hpp"

struct BootInfo {
    // This assumes our kernel will be ELF forever - this might not be
    // true on UEFI? (I think UEFI binaries are PE)
    struct Elf {
        size_t shnum;
        size_t shndx;

        // TODO: This should be an owned pointer
        Elf64::SectionHeader* sections;
    } elf;

    String cmdline;
};
