#pragma once

#include "stddef.h"
#include "stdint.h"

class Allocator {
public:
    static Allocator& global();

    void add_region(uintptr_t start, size_t size);

    void* alloc(size_t size);
    void dealloc(void* addr);

    void dump() const;

private:
    struct FreeChunk {
        size_t size;
        FreeChunk* next;
    };

    FreeChunk* m_head;
};
