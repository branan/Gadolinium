#include "multiboot2.hpp"
#include "kmemlayout.h"
#include "logging.hpp"
#include "allocator.hpp"
#include "page_allocator.hpp"

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
    "acpi (old)",
    "acpi (new)",
    "network",

    // TODO: There are some newer multiboot tags in modern GRUBs. We
    // should know how to print them.
};

// The multiboot tags we care about, broken out for us to parse later
struct multiboot {
    const Multiboot::TagMmap* mmap = 0;
    const Multiboot::TagElfSections* shdr = 0;
    const Multiboot::TagOldAcpi *acpi_old = 0;
    const Multiboot::TagNewAcpi *acpi_new = 0;
    const Multiboot::TagCommandLine *cmd_line = 0;
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
        EXTRACT_TAG(cmd_line, CommandLine);
    }

#define CHECK_TAG(field, class) \
    if (mb->field == nullptr) { \
        klog("Missing multiboot tag of type ", tag_names[Multiboot::Tag##class::Id], "\n"); \
        return false; \
    }

    CHECK_TAG(mmap, Mmap);
    CHECK_TAG(shdr, ElfSections);
    CHECK_TAG(cmd_line, CommandLine);

    // special case - we only need one of these
    if(mb->acpi_old == nullptr && mb->acpi_new == nullptr) {
        klog("Missing multiboot tag of type ",
             tag_names[Multiboot::TagOldAcpi::Id], " or ",
             tag_names[Multiboot::TagNewAcpi::Id], "\n");
        return false;
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

    for (const auto& section : *mb.shdr) {
        if (section.type && section.addr) {
            auto addr = section.addr > KERNEL_VMA_BASE ? section.addr - KERNEL_VMA_BASE : section.addr;
            alloc.reserve_region(addr, section.size);
        }
    }

    return true;
}

// TODO: Should return a pointer to a kernel information struct (or
// nullptr on error). Should include suficiently extracted information
// for kmain to do what it needs to do.
extern "C" bool kinit(uint32_t magic, uint32_t multiboot_ptr) {
    if(MULTIBOOT2_BOOTLOADER_MAGIC != magic) {
        klog("Not loaded from a multiboot2-compliant bootloader");
        return false;
    }

    Allocator::global().add_region(KERNEL_HEAP_START, 0x8000);

    // We want to stash a few values from the multiboot tags before we
    // initialize our memory management, as we will likely overwrite
    // Grub's data structures if we don't.
    multiboot tags;
    if (!read_multiboot_tags((uintptr_t)multiboot_ptr, &tags)) {
        return false;
    }

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
    // initial ones. This will put the kernel in HIGH MEMORY ONLY and
    // any device memory will need to be mapped (with appropriate
    // cache settings).

    // TODO: Should re-mapping and re-initializing the logger (if we
    // switch to VGA) be done here?

    // The things below probably belong in kmain?
    // TODO: setup interrupts/error handling
    // TODO: initialize userspace
    // TODO: initialize drivers

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

// TODO: Should this print to the VGA text console instead of serial?
Sink* Sink::global() {
    static SerialSink s;
    return &s;

}
