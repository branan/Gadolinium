#include "boot_information.hpp"
#include "logging.hpp"

extern "C" void kmain(const BootInfo* boot_info) {
    klog(boot_info->cmdline, "\n");
}
