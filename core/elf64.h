#pragma once

#include "stdint.h"

struct Elf64Section {
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
};
