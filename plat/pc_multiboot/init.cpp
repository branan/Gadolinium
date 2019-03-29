#include "multiboot2.hpp"
#include "kmemlayout.h"
#include "logging.hpp"
#include "allocator.hpp"
#include "page_allocator.hpp"
#include "elf64.h"

static const char* tag_names[] = {
    "end",
    "cmdline",
    "boot loader name",
    "module",
    "basic meminfo",
    "bootdev",
    "mmap",
    "vbe",
    "framebuffer",
    "elf sections",
    "apm",
    "efi32",
    "efi64",
    "smbios",
    "acpi old",
    "acpi new",
    "network",
};

// The multiboot tags we care about, broken out for us to parse later
struct multiboot {
    const Multiboot::TagMmap* mmap = 0;
    const Multiboot::TagElfSections* shdr = 0;
    const Multiboot::TagOldAcpi *acpi_old = 0;
    const Multiboot::TagNewAcpi *acpi_new = 0;
};

bool read_multiboot_tags(uintptr_t descriptor, multiboot* mb) {
    auto info = Multiboot::Information(descriptor);
    for(const auto& tag : info) {

#define EXTRACT_TAG(field, class)                       \
        auto field = tag.cast<Multiboot::Tag##class>(); \
        if (field) { mb->field = field; continue; }

        EXTRACT_TAG(mmap, Mmap);
        EXTRACT_TAG(shdr, ElfSections);
        EXTRACT_TAG(acpi_old, OldAcpi);
        EXTRACT_TAG(acpi_new, NewAcpi);

        auto biggest_tag = sizeof(tag_names) / sizeof(tag_names[0]);
        if (tag.type > biggest_tag) {
            klog("Ignorning an unknown multiboot tag of type ", tag.type, "\n");
        } else {
            klog("Ignorning a multiboot tag of type ", tag_names[tag.type], "\n");
        }
    }
    return true;
}

bool initialize_page_allocator (multiboot& mb, PageAllocator& alloc) {
    for (const auto& entry : *mb.mmap) {
        if (entry.type == Multiboot::TagMmap::Entry::Type::Available) {
            alloc.add_region(entry.addr, entry.len);
        }
    }
    for (const auto& entry : *mb.mmap) {
        if (entry.type != Multiboot::TagMmap::Entry::Type::Available) {
            alloc.reserve_region(entry.addr, entry.len);
        }
    }

    for (size_t i = 0; i < mb.shdr->num; i++) {
        auto section = (Elf64Section*)(mb.shdr->sections + i * mb.shdr->entsize);
        if (section->type && section->addr) {
            auto addr = section->addr > KERNEL_VMA_BASE ? section->addr - KERNEL_VMA_BASE : section->addr;
            alloc.reserve_region(addr, section->size);
        }
    }
    klog("\n");

    return true;
}

void print_mmap(multiboot& mb) {
    static const char* labels[] =
        { "",
          "available",
          "reserved",
          "reclaimable"
          "nonvolatile"
          "bad"
        };

    klog("==Physical Memory Map==\n");
    size_t num_entries = (mb.mmap->size - 16) / mb.mmap->entry_size;
    for(size_t i = 0; i < num_entries; i++) {
        auto entry = mb.mmap->entries[i];
        klog(entry.addr, ":", entry.addr+entry.len, " ", labels[(uint32_t)entry.type], "\n");
    }
}

struct rsdt;
struct xsdt;

extern "C" bool kinit(uint32_t magic, uint32_t multiboot_ptr) {
    if(MULTIBOOT2_BOOTLOADER_MAGIC != magic) {
        klog("Not loaded from a multiboot2-compliant bootloader");
        return false;
    }

    klog(multiboot_ptr, "\n\n");

    Allocator::global().add_region(KERNEL_HEAP_START, 0x8000);

    // We want to stash a few values from the multiboot tags before we
    // initialize our memory management, as we will likely overwrite
    // Grub's data structures if we don't.
    multiboot tags;
    read_multiboot_tags((uintptr_t)multiboot_ptr, &tags);

    if (tags.acpi_new) {
        // TODO: Get XSDT pointer
    } else if (tags.acpi_old) {
        // TODO: Get RSDT pointer
    } else {
        klog("Could not find ACPI tables.");
        return false;
    }

    size_t elf_header_size = tags.shdr->num * tags.shdr->entsize;
    uint8_t* elf_section_headers = new uint8_t[elf_header_size];
    for (size_t i = 0; i < elf_header_size; i++)
        elf_section_headers[i] = tags.shdr->sections[i];

    if(!initialize_page_allocator(tags, PageAllocator::global())) {
        return false;
    }

    // Below here, we risk clobbering any multiboot data, since we're
    // allocating pages.

    // TODO: create our real tables, and reclaim pages occupied by
    // initial ones.
    // TODO: setup interrupts/error handling
    // TODO: initialize userspace
    // TODO: initialize drivers

    Allocator::global().dump();
    klog("\n");
    PageAllocator::global().dump();
    klog("\n");
    print_mmap(tags);

    return true;
}

struct SerialSink : public Sink {
    SerialSink() {
        asm volatile ("outb %b0, %w1" :: "a"(0x03), "Nd"(0x3fb));
        asm volatile ("outb %b0, %w1" :: "a"(0xC7), "Nd"(0x3fa));
    }

    void writec(char c) {
        uint8_t status;
        do {
            asm volatile("inb %w1, %b0" : "=a"(status) : "Nd"(0x3fd));
        } while ((status & 0x20) == 0);
        asm volatile ("outb %b0, %w1" :: "a"(c), "Nd"(0x3f8));
    }
};

Sink* Sink::global() {
    static SerialSink s;
    return &s;

}
