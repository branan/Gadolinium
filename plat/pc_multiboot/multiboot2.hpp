#pragma once

#define MULTIBOOT_HEADER_ALIGN 8

/* The magic field should contain this.  */
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6

/* This should be in %eax.  */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

#define MULTIBOOT_TAG_ALIGN                 8

#define MULTIBOOT_HEADER_TAG_END                 0
#define MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST 1
#define MULTIBOOT_HEADER_TAG_ADDRESS             2
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS       3
#define MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS       4
#define MULTIBOOT_HEADER_TAG_FRAMEBUFFER         5
#define MULTIBOOT_HEADER_TAG_MODULE_ALIGN        6

#define MULTIBOOT_ARCHITECTURE_I386   0
#define MULTIBOOT_ARCHITECTURE_MIPS32 4
#define MULTIBOOT_HEADER_TAG_OPTIONAL 1

#define MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED   1
#define MULTIBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED 2

#ifndef ASM_FILE

#include "stddef.h"
#include "stdint.h"

namespace Multiboot {
    struct Tag {
        uint32_t type;
        uint32_t size;

        template <typename T>
        const T* cast() const {
            if (type == T::Id) {
                return (T*)this;
            } else {
                return nullptr;
            }
        }
    };

    class Information {
        struct Iterator {
            uintptr_t cursor;
            uintptr_t end;

            void operator++() {
                cursor += (*this)->size;
                if (cursor % MULTIBOOT_TAG_ALIGN != 0) {
                    cursor += MULTIBOOT_TAG_ALIGN;
                    cursor &= ~(MULTIBOOT_TAG_ALIGN - 1);
                }
                if (cursor > end) {
                    cursor = end;
                }
            }

            bool operator !=(Iterator& other) const {
                return cursor != other.cursor;
            }

            const Tag& operator*() const {
                return *(Tag*)cursor;
            }

            const Tag* operator->() const {
                return (Tag*)cursor;
            }
        };

        struct Header {
            uint32_t size;
            uint32_t reserved;
        };

        uintptr_t address;

    public:
        Information(uintptr_t address) : address(address) {}

        Iterator begin() const {
            auto end = address + reinterpret_cast<Header*>(address)->size;
            return Iterator {address + sizeof(Header), end};
        }

        Iterator end() const {
            auto end = address + reinterpret_cast<Header*>(address)->size;
            return Iterator {end, end};
        }
    };

    struct TagMmap {
        static constexpr int Id = 6;

        struct Entry {
            uint64_t addr;
            uint64_t len;
            enum class Type : uint32_t {
                Available = 1,
                Reserved = 2,
                AcpiReclaimable = 3,
                NonVolatileStorage = 4,
                BadRam = 5
            };
            Type type;
            uint32_t zero;
        } __attribute__((packed));

        uint32_t type;
        uint32_t size;
        uint32_t entry_size;
        uint32_t entry_version;
        Entry entries[0];

        struct Iterator {
            const Entry* entry;
            const Entry* end;
            uint32_t entry_size;

            const Entry& operator*() const {
                return *entry;
            }

            const Entry* operator->() const {
                return entry;
            }

            void operator++() {
                entry = (Entry*)((uintptr_t)entry + entry_size);
                if (entry > end)
                    entry = end;
            }

            bool operator !=(Iterator& other) const {
                return entry != other.entry;
            }
        };

        Iterator begin() const {
            auto count = (size - 16) / entry_size;
            auto end = (Entry*)((uintptr_t)entries + count * entry_size);
            return Iterator {entries, end, entry_size};
        }

        Iterator end() const {
            auto count = (size - 16) / entry_size;
            auto end = (Entry*)((uintptr_t)entries + count * entry_size);
            return Iterator {end, end, entry_size};
        }
    };

    struct TagElfSections {
        static constexpr int Id = 9;

        uint32_t type;
        uint32_t size;
        uint32_t num;
        uint32_t entsize;
        uint32_t shndx;
        char sections[0];
    };

    struct TagOldAcpi {
        static constexpr int Id = 14;

        uint32_t type;
        uint32_t size;
        uint8_t rsdp[0];
    };

    struct TagNewAcpi {
        static constexpr int Id = 15;

        uint32_t type;
        uint32_t size;
        uint8_t rsdp[0];
    };
}

#endif
