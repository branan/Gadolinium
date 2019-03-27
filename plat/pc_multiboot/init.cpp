#include "multiboot2.h"
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
    multiboot_tag_mmap* mmap = 0;
    multiboot_tag_elf_sections* shdr = 0;
    multiboot_tag_old_acpi *acpi_old = 0;
    multiboot_tag_new_acpi *acpi_new = 0;
};

bool read_multiboot_tags(const uint8_t* descriptor, multiboot* mb) {
    auto next_tag = [&](const multiboot_tag* tag) {
        auto size = tag->size;
        if(size % 8 != 0) {
            size += 8;
            size &= 0xfffffff8;
        }
        descriptor += size;
        return (multiboot_tag*)descriptor;
    };

    auto info = (multiboot_information*)descriptor;
    const uint8_t* desc_end = descriptor + info->size;
    descriptor += 8; // skip past the info header
    for(auto tag = (multiboot_tag*)descriptor;
        descriptor < desc_end && tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = next_tag(tag)) {
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_MMAP:
            mb->mmap = (multiboot_tag_mmap*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
            mb->shdr = (multiboot_tag_elf_sections*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            mb->acpi_old = (multiboot_tag_old_acpi*)tag;
            break;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
            mb->acpi_new = (multiboot_tag_new_acpi*)tag;
            break;
        default:
            if (tag->type > MULTIBOOT_TAG_TYPE_NETWORK) {
                klog("Ignoring an unknown multiboot tag of type ", tag->type, "\n");
            } else {
                klog("Ignorning multiboot tag of type ", tag_names[tag->type], "\n");
            }
        }
    }
    return true;
}

bool initialize_page_allocator (multiboot& mb, PageAllocator& alloc) {
    if (mb.mmap->entry_size != sizeof(multiboot_mmap_entry)) {
        klog("Can't use the memory map!");
        return false;
    }
    uint64_t num_entries = (mb.mmap->size - 16) / mb.mmap->entry_size;
    for(uint64_t i = 0; i < num_entries; i++) {
        auto entry = mb.mmap->entries[i];
        if (entry.type == MULTIBOOT_MEMORY_AVAILABLE) {
            alloc.add_region(entry.addr, entry.len);
        }
    }
    for(uint64_t i = 0; i < num_entries; i++) {
        auto entry = mb.mmap->entries[i];
        if (entry.type == MULTIBOOT_MEMORY_RESERVED ||
            entry.type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE ||
            entry.type == MULTIBOOT_MEMORY_NVS ||
            entry.type == MULTIBOOT_MEMORY_BADRAM) {
            alloc.reserve_region(entry.addr, entry.len);
        }
    }

    for (uint64_t i = 0; i < mb.shdr->num; i++) {
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
    uint64_t num_entries = (mb.mmap->size - 16) / mb.mmap->entry_size;
    for(uint64_t i = 0; i < num_entries; i++) {
        auto entry = mb.mmap->entries[i];
        klog(entry.addr, ":", entry.addr+entry.len, " ", labels[entry.type], "\n");
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
    read_multiboot_tags((uint8_t*)(uint64_t)multiboot_ptr, &tags);

    if (tags.acpi_new) {
        // TODO: Get XSDT pointer
    } else if (tags.acpi_old) {
        // TODO: Get RSDT pointer
    } else {
        klog("Could not find ACPI tables.");
        return false;
    }

    uint64_t elf_header_size = tags.shdr->num * tags.shdr->entsize;
    uint8_t* elf_section_headers = new uint8_t[elf_header_size];
    for (uint64_t i = 0; i < elf_header_size; i++)
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
