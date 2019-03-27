#pragma once

#include "stdint.h"
#include "list.hpp"

class PageAllocator {
public:
    static PageAllocator& global();

    void add_region(uint64_t start, uint64_t size);
    void reserve_region(uint64_t start, uint64_t size);

    uint64_t alloc(uint64_t size);

    void dump() const;
private:
    struct PageChunk {
        uint64_t start;
        uint64_t size;
    };

    List<PageChunk> m_huge_chunks;
    List<PageChunk> m_big_chunks;
    List<PageChunk> m_chunks;

    void add_chunk(List<PageChunk>& chunks, uint64_t start, uint64_t size);
};
