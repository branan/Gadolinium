#pragma once

#include "stddef.h"
#include "stdint.h"

class PageTable {
    uintptr_t pml4;

public:
    PageTable create();
    PageTable current();

    void map(uintptr_t phys, uintptr_t virt, size_t size);
    void unmap(uintptr_t virt, size_t size);
    uintptr_t physical(uintptr_t virt);

    void activate();
};
