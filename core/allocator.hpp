#pragma once

#include "stdint.h"

class Allocator {
public:
    static Allocator& global();

    void add_region(uint64_t start, uint64_t size);

    void* alloc(uint64_t size);
    void dealloc(void* addr);

    void dump() const;

private:
    struct FreeChunk {
        uint64_t size;
        FreeChunk* next;
    };

    FreeChunk* m_head;
};
