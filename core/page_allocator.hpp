#pragma once

#include "stddef.h"
#include "stdint.h"
#include "list.hpp"

class PageAllocator {
public:
    static PageAllocator& global();

    void add_region(uintptr_t start, size_t size);
    void reserve_region(uintptr_t start, size_t size);

    uintptr_t alloc(size_t size);

    void dump() const;
private:
    struct PageChunk {
        uintptr_t start;
        size_t size;
    };

    List<PageChunk> m_huge_chunks;
    List<PageChunk> m_big_chunks;
    List<PageChunk> m_chunks;

    void add_chunk(List<PageChunk>& chunks, uintptr_t start, size_t size);
};
