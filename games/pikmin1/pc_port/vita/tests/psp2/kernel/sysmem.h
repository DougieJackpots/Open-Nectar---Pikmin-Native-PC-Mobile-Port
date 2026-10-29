#pragma once
// Host test double. Production compilation uses the VitaSDK header.
#include <cstddef>
#include <cstdint>
struct SceKernelMemBlockInfo {
    size_t size;
    void* mappedBase;
    size_t mappedSize;
    int memoryType;
    uint32_t access;
    int type;
};
constexpr uint32_t SCE_KERNEL_MEMORY_ACCESS_R = 0x04;
int sceKernelGetMemBlockInfoByAddr(void*, SceKernelMemBlockInfo*);
