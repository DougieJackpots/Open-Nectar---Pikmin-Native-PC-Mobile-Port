#include "dolphin_stubs/pc_host_readable_os.h"
#include <psp2/kernel/sysmem.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <limits>

static SceKernelMemBlockInfo block;
static int queryResult = 0;
static int queryCount = 0;
static int failures = 0;

int sceKernelGetMemBlockInfoByAddr(void*, SceKernelMemBlockInfo* info)
{
    ++queryCount;
    if (info->size != sizeof(*info)) std::abort();
    *info = block;
    return queryResult;
}

static void check(uintptr_t address, PcHostReadable expected, uintptr_t expectedEnd, const char* name)
{
    const void* end = reinterpret_cast<void*>(0xBAD);
    const auto result = pc_host_os_readable_end(reinterpret_cast<void*>(address), 64, &end);
    if (result != expected || reinterpret_cast<uintptr_t>(end) != expectedEnd) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        ++failures;
    }
}

int main()
{
    block.mappedBase = reinterpret_cast<void*>(0x1000);
    block.mappedSize = 0x1000;
    block.access = SCE_KERNEL_MEMORY_ACCESS_R;
    check(0, PcHostReadable::No, 0, "null pointer");
    if (queryCount != 0) ++failures;
    check(0x1000, PcHostReadable::Yes, 0x2000, "block start");
    check(0x1FFF, PcHostReadable::Yes, 0x2000, "last readable byte");
    check(0x2000, PcHostReadable::No, 0, "exclusive end");
    check(0xFFF, PcHostReadable::No, 0, "before block");
    block.access = 2;
    check(0x1000, PcHostReadable::No, 0, "block without read permission");
    block.access = SCE_KERNEL_MEMORY_ACCESS_R;
    queryResult = -1;
    check(0x1000, PcHostReadable::No, 0, "failed lookup after successful lookup");
    queryResult = 0;
    block.mappedSize = 0;
    check(0x1000, PcHostReadable::No, 0, "empty block");
    block.mappedBase = reinterpret_cast<void*>(std::numeric_limits<uintptr_t>::max() - 15);
    block.mappedSize = 32;
    check(std::numeric_limits<uintptr_t>::max() - 1, PcHostReadable::No, 0, "overflowed block");
    std::printf("Vita memory query: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
