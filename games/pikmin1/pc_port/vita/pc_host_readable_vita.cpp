// Vita has no /proc/self/maps. Query a real mapped block instead of treating
// every host pointer as unreadable (or trusting arbitrary pointer values).
#include "dolphin_stubs/pc_host_readable_os.h"

#include <cstdint>
#include <limits>
#include <psp2/kernel/sysmem.h>

PcHostReadable pc_host_os_readable_end(const void* p, size_t, const void** end)
{
    *end = nullptr;
    if (!p) return PcHostReadable::No;

    SceKernelMemBlockInfo info = {};
    info.size = sizeof(info);
    if (sceKernelGetMemBlockInfoByAddr(const_cast<void*>(p), &info) < 0
        || !(info.access & SCE_KERNEL_MEMORY_ACCESS_R))
        return PcHostReadable::No;

    const uintptr_t base = reinterpret_cast<uintptr_t>(info.mappedBase);
    const uintptr_t address = reinterpret_cast<uintptr_t>(p);
    if (address < base || info.mappedSize > std::numeric_limits<uintptr_t>::max() - base)
        return PcHostReadable::No;
    const uintptr_t limit = base + info.mappedSize;
    if (address >= limit) return PcHostReadable::No;

    // A block is a conservative bound: no assumption that adjacent blocks are
    // readable, and no cache that can outlive a freed/reused allocation.
    *end = reinterpret_cast<const void*>(limit);
    return PcHostReadable::Yes;
}
