#include "page_allocator.hpp"
#include "logging.hpp"
#include "kmemlayout.h"

PageAllocator& PageAllocator::global() {
    static PageAllocator instance;
    return instance;
}

void PageAllocator::add_region(uintptr_t start, size_t size) {
    uintptr_t huge_start = start;
    uintptr_t huge_end = start + size;
    uintptr_t big_start = start;
    uintptr_t big_end = start + size;
    uintptr_t end = start + size;

    // align huge section start/end
    if (huge_start & HUGE_PAGE_MASK) {
        huge_start += HUGE_PAGE_SIZE;
        huge_start &= ~HUGE_PAGE_MASK;
    }
    huge_end &= ~HUGE_PAGE_MASK;

    // align big section start/end
    if (big_start & BIG_PAGE_MASK) {
        big_start += BIG_PAGE_SIZE;
        big_start &= ~BIG_PAGE_MASK;
    }
    big_end &= ~BIG_PAGE_MASK;

    // align page section start/end
    if (start & PAGE_MASK) {
        start += PAGE_SIZE;
        start &= ~PAGE_MASK;
    }
    end &= ~PAGE_MASK;

    // If there's a huge-aliged section of this region, we can add it first
    if (huge_start < huge_end) {
        add_chunk(m_huge_chunks, huge_start, huge_end-huge_start);

        // We now need to check if we have any big/normal regions on
        // the edges of the huge region.
        if (big_start < huge_start) {
            add_chunk(m_big_chunks, big_start, huge_start - big_start);
            if (start < big_start) {
                add_chunk(m_chunks, start, big_start - start);
            }
        } else {
            if (start < huge_start) {
                add_chunk(m_chunks, start, huge_start - start);
            }
        }

        if (big_end > huge_end) {
            add_chunk(m_big_chunks, huge_end, big_end - huge_end);
            if (end > big_end) {
                add_chunk(m_chunks, big_end, end - big_end);
            }
        } else {
            if (end > huge_end) {
                add_chunk(m_chunks, end, end - huge_end);
            }
        }
    } else if (big_start < big_end) {
        add_chunk(m_big_chunks, big_start, big_end - big_start);

        if (end > big_end) {
            add_chunk(m_chunks, big_end, end - big_end);
        }

        if (start < big_start) {
            add_chunk(m_chunks, start, big_start - start);
        }
    } else if (start < end) {
        add_chunk(m_chunks, start, end - start);
    }
}

void PageAllocator::add_chunk(List<PageChunk>& chunks, uintptr_t start, size_t size) {
    uintptr_t end = start + size;

    // TODO: lock the page allocator for multi-cpu safety
    auto chunk = chunks.begin();
    for (; chunk != chunks.end(); ++chunk) {
        // If we belong directly after this chunk, we'll remove it
        // from the list and update our start, size, and end. The next
        // iterator step will re-insert us in the right place - either
        // merging with the next chunk if we span the gap, or
        // inserting us just before it
        if (chunk->start + chunk->size == start) {
            start = chunk->start;
            size += chunk->size;
            chunk.remove();
            continue;
        }

        // If we belong directly before this chunk, update the start+size;
        if (end == chunk->start) {
            chunk->start = start;
            chunk->size = size + chunk->size;
            return;
        }

        // If we are entirely before this chunk, insert ourselves into
        // the list here.
        if (end < chunk->start) {
            break;
        }
    }

    // If we get here, we've either iterated through the entire list
    // (and are adding ourselves to the end), or we broke where we
    // belong.
    chunk.insert({start, size});
}

void PageAllocator::reserve_region(uintptr_t start, size_t size) {
    // For each type of chunk, we check for overlap with the
    // region. If we find any, we trim the chunk, then re-add what's
    // left
    auto end = start + size;

    auto trim_chunks = [&](List<PageChunk>& chunks) {
        for(auto chunk = chunks.begin(); chunk != chunks.end(); ++chunk) {
            auto chunk_end = chunk->start + chunk->size;
            if (chunk->start >= end) {
                // We've gone past any chunks this region would affect;
                break;
            }
            if (chunk_end <= start) {
                continue;
            }
            // If we get here it means that we have some overlap with the chunk
        
            if (start >= chunk->start && end <= chunk_end) {
                // The reserved region is entirely contained within this chunk. Split it.
                // Start by removing this chunk from the list
                PageChunk self = *chunk;
                chunk.remove();
            
                add_region(self.start, start - self.start);
                add_region(end, chunk_end - end);
                // Since we were entirely contained, we're done
                break;
            }
            if (start < chunk->start) {
                // The resserved region overlaps the start of this chunk.
                chunk.remove();
                add_region(end, chunk_end - end);
                // We've hit the end of our reserved region, so we can
                // break here.
                break;
            }
            if (end > chunk_end) {
                // The reserved region overlaps the end of this chunk
                PageChunk self = *chunk;
                chunk.remove();
                add_region(self.start, start - self.start);
                // We need to be sure the end doesn't overlap with the
                // next region, so we continue here.
                continue;
            }
        }
    };

    trim_chunks(m_huge_chunks);
    trim_chunks(m_big_chunks);
    trim_chunks(m_chunks);
}

void PageAllocator::dump() const {
    klog("==Page Allocator==\n");
    klog("*Huge Pages*\n");
    for (auto& chunk : m_huge_chunks) {
        klog(chunk.start, ":", chunk.start+chunk.size, "\n");
    }
    klog("\n*Big Pages*\n");
    for (auto& chunk : m_big_chunks) {
        klog(chunk.start, ":", chunk.start+chunk.size, "\n");
    }
    klog("\n*Pages*\n");
    for (auto& chunk : m_chunks) {
        klog(chunk.start, ":", chunk.start+chunk.size, "\n");
    }
}
