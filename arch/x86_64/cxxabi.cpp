#include "stdint.h"

#include "logging.hpp"
#include "allocator.hpp"

#define GUARD_INITIALIZED 1
#define GUARD_ENABLE_INTERRUPTS 2

struct Guard {
    uint32_t m_lock;
    uint32_t m_status;

    void lock() {
        // TODO: disable interrupts to avoid reentrency here, and flag
        // in the Guard structure if needed.
        while(__sync_bool_compare_and_swap(&m_lock, 0, 1) == false) {}
    }

    void unlock() {
        __sync_bool_compare_and_swap(&m_lock, 1, 0);
        if (m_status & GUARD_ENABLE_INTERRUPTS) {
            // TODO: enable interrupts
        }
    }

    void initialize() {
        m_status |= GUARD_INITIALIZED;
    }

    bool initialized() {
        return m_status & GUARD_INITIALIZED;
    }
};

extern "C" int __cxa_guard_acquire(Guard* guard) {
    if(guard->initialized()) {
        return 0;
    }

    guard->lock();
    if(guard->initialized()) {
        guard->unlock();
        return 0;
    }
    return 1;
}

extern "C" void __cxa_guard_release(Guard* guard) {
    guard->initialize();
    guard->unlock();
}

extern "C" void __cxa_guard_abort(Guard* guard) {
    guard->unlock();
}

extern "C" void __cxa_pure_virtual() {
    klog("Called a virtual function. Halting...");
    // TODO: Panic?
    while(true) {};
}

void* operator new(unsigned long size) throw() {
    return Allocator::global().alloc(size);
}

void* operator new[](unsigned long size) throw() {
    return Allocator::global().alloc(size);
}

void operator delete(void* ptr) throw() {
    Allocator::global().dealloc(ptr);
}

void operator delete(void* ptr, unsigned long) throw() {
    Allocator::global().dealloc(ptr);
}
