project(gadolinium)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IMAGE_FILE
  ${CMAKE_BINARY_DIR}/gadolinium.iso
  CACHE PATH "Output path for ISO image"
)

set(PLATFORM
  "pc_multiboot"
  CACHE STRING "The type of boot environment to target"
)

set(ARCH
  "x86_64"
  CACHE STRING "The target architecture"
)

if (PLATFORM STREQUAL "pc_multiboot")
  set(plat_SOURCES
    plat/pc_multiboot/multiboot.S
    plat/pc_multiboot/init.cpp
  )
  set_property(SOURCE plat/pc_multiboot/multiboot.S PROPERTY LANGUAGE C)
  include_directories(plat/pc_multiboot)
  set(OUTPUT ${CMAKE_BINARY_DIR}/isofiles/boot)
else()
  message(FATAL_ERROR "Unknown platform: ${PLATFORM}")
endif()

if (ARCH STREQUAL "x86_64")
  set(arch_SOURCES
    arch/x86_64/tables.S
    arch/x86_64/startup.S
    arch/x86_64/cxxabi.cpp
  )
  set_property(SOURCE arch/x86_64/tables.S PROPERTY LANGUAGE C)
  set_property(SOURCE arch/x86_64/startup.S PROPERTY LANGUAGE C)
  include_directories(arch/x86_64)
else()
  message(FATAL_ERROR "Unknown architecture: ${ARCH}")
endif()

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/${ARCH}-${PLATFORM}-gadolinium.ld")

set(kernel_SOURCES
  ${plat_SOURCES}
  ${arch_SOURCES}

  core/logging.cpp
  core/allocator.cpp
  core/page_allocator.cpp
)

include_directories(core)

add_executable(kernel ${kernel_SOURCES})

# Common options. Platform/arch specific options are added below.
target_compile_options(kernel PUBLIC -fno-stack-protector -fno-rtti -fno-exceptions -fno-builtin -nostdlib -Wall -Wextra -Werror)

set_target_properties(kernel PROPERTIES
  LINK_DEPENDS "${LINKER_SCRIPT}"
  LINK_FLAGS "-m64 -static -T${LINKER_SCRIPT} -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -z max-page-size=0x1000 -Wl,--build-id=none -Wl,--no-dynamic-linker"
  RUNTIME_OUTPUT_DIRECTORY ${OUTPUT}
)

# Setup image and run/dbg targets

# The image contents are platform-specific.
if (PLATFORM STREQUAL "pc_multiboot")
  configure_file(plat/pc_multiboot/grub.cfg.in ${OUTPUT}/grub/grub.cfg)

  add_custom_command(OUTPUT ${IMAGE_FILE}
    COMMAND grub-mkrescue -o ${IMAGE_FILE} ${CMAKE_BINARY_DIR}/isofiles
    DEPENDS kernel ${OUTPUT}/grub/grub.cfg
    )

  target_compile_options(kernel PUBLIC -fno-PIC)
endif()

add_custom_target(image ALL DEPENDS ${IMAGE_FILE})

if (ARCH STREQUAL "x86_64")
  set(QEMU_OPTIONS -machine q35 -soundhw hda -serial stdio -m 4G -smp 2 -cdrom ${IMAGE_FILE})

  target_compile_options(kernel PUBLIC -m64 -mcmodel=kernel)
endif()

add_custom_target(run
  COMMAND qemu-system-${ARCH} ${QEMU_OPTIONS}
  DEPENDS image
)

add_custom_target(dbg
   COMMAND qemu-system-${ARCH} ${QEMU_OPTIONS} -s -S
   DEPENDS image
)

add_custom_target(gdb
  COMMAND gdb ${CMAKE_BINARY_DIR}/isofiles/boot/kernel -ex "target remote localhost:1234"
  DEPENDS kernel
)
