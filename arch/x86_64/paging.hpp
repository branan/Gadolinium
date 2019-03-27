#pragma once

#include "stdint.h"

class PageTable {
    uint64_t pml4;

public:
    PageTable create();
    PageTable current();

    void map(uint64_t phys, uint64_t virt, uint64_t size);
    void unmap(uint64_t virt, uint64_t size);
    void physical(void* virt);

    void activate();
};
