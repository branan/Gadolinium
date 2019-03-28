#include "kmemlayout.h"

#include "allocator.hpp"
#include "logging.hpp"

Allocator& Allocator::global() {
    static Allocator instance;
    return instance;
}

void Allocator::add_region(uintptr_t addr, size_t size) {
    FreeChunk* prev = nullptr;
    FreeChunk* chunk = m_head;

    // iterate through the freelist to find the best insertion point.
    // The list is sorted, so this also tries to merge adjacent
    // regions if possible.
    while (chunk) {
        if (addr+size < (uintptr_t)chunk) {
            // we're contained entirely before this chunk.
            // Make sure we're big enough to be a standalone chunk
            if (size < ALLOC_ALIGN * 2) {
                return;
            }
            auto new_chunk = (FreeChunk*)addr;
            new_chunk->size = size;
            new_chunk->next = chunk;
            (prev ? prev->next : m_head) = new_chunk;
            return;
        } else if ((uintptr_t)chunk + chunk->size == addr) {
            // We are just after this chunk. We'll update our start
            // address+size, and use the fallthrough paths to merge us
            // if necessary;
            addr = (uintptr_t)chunk;
            size += chunk->size;
            (prev? prev->next : m_head) = chunk->next;
            chunk = chunk->next;
        } else if (addr + size == (uintptr_t) chunk) {
            // We are just before this chunk. We'll update our start
            // address + size, and use the fallthrough paths to insert
            // us where we belong.
            size += chunk->size;
            (prev ? prev->next : m_head) = chunk->next;
            chunk = chunk->next;
        } else {
            prev = chunk;
            chunk = chunk->next;
        }
    }
    
    // If we iterated through the entire list, we are contained
    // entirely after the existing chunks.
    if (size < ALLOC_ALIGN * 2) {
        // Too small to use as a standalone allocation, just leak for
        // now.
        return;
    }
    auto new_chunk = (FreeChunk*)addr;
    new_chunk->size = size;
    new_chunk->next = nullptr;
    (prev ? prev->next : m_head) = new_chunk;
}

void* Allocator::alloc(size_t size) {
    // Round up size to a multiple of our alloc alignment
    if ((size & ALLOC_MASK) != 0) {
        size += ALLOC_ALIGN;
        size &= ~ALLOC_MASK;
    }
    // add space for our size counter and sentinal value
    size += ALLOC_ALIGN;

    // Find a chunk big enough to hold our allocation, and chop an
    // allocation off the end of it.
    FreeChunk* prev = nullptr;
    auto chunk = m_head;
    while (chunk) {
        if (chunk->size >= size) {
            chunk->size -= size;
            size_t* block = nullptr;
            if (chunk->size < (ALLOC_ALIGN*2)) {
                // We've consumed this entire chunk.
                size += chunk->size;
                block = (size_t*)chunk;
                (prev ? prev->next : m_head) = chunk->next;
            } else {
                // otherwise we are somewhere after the new chunk
                block = (size_t*)((uintptr_t)chunk + chunk->size);
            }
            block[0] = size;
            block[1] = 0xDECAF'00000'C0FFEE;
            return (void*)(&block[2]);
        }
        prev = chunk;
        chunk = chunk->next;
    }
    // TODO: Request a new chunk of virtual memory, and pages to populate it.
    // TOOD: How much should we allocate at a time?
    klog("Out of memory to allocate!");
    while(true) {};
}

void Allocator::dealloc(void* addr) {
    auto block_addr = (uintptr_t)addr - ALLOC_ALIGN;
    auto block = (uintptr_t*)block_addr;
    if (block[1] != 0xDECAF'00000'C0FFEE) {
        klog("Tried to free a non-allocated region. Ignoring\n");
        return;
    }
    add_region(block_addr, block[0]);
}

void Allocator::dump() const {
    klog("==Heap Allocator==\n");
    auto chunk = m_head;
    while(chunk) {
        klog((uintptr_t)chunk,":", chunk->size+(uintptr_t)chunk, "\n");
        chunk = chunk->next;
    }
}
