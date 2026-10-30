#include "pc_vita_runtime.h"
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <psp2/kernel/sysmem.h>

// Reserve a bounded C/C++ heap rather than using the desktop's virtual-memory
// assumptions. This is a provisional device budget, not a measured peak.
extern "C" { unsigned int _newlib_heap_size_user = 128 * 1024 * 1024; }

namespace {
bool ensure_directory(const char* path)
{
    struct stat st;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode);
    return mkdir(path, 0777) == 0;
}
bool fail(const char* message)
{
    std::fprintf(stderr, "[Vita] %s\n", message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Nectar Vita Dev", message, nullptr);
    return false;
}
}

bool pc_vita_init_storage()
{
    if (!ensure_directory("ux0:data") || !ensure_directory("ux0:data/OpenNectar")
        || !ensure_directory("ux0:data/OpenNectar/pikmin1")
        || chdir("ux0:data/OpenNectar/pikmin1") != 0)
        return fail("Cannot open ux0:data/OpenNectar/pikmin1 for game data.");

    if (!std::freopen("pikmin.log", "a", stdout)) return fail("Cannot open pikmin.log.");
    if (!std::freopen("pikmin.log", "a", stderr)) return false;
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    std::puts("\n[Vita] Experimental game startup; not validated for gameplay.");
    std::puts("[Vita] Game target: 30 FPS / 33.33 ms; display refresh is separate.");
    SceKernelFreeMemorySizeInfo memory = {};
    memory.size = sizeof(memory);
    if (sceKernelGetFreeMemorySize(&memory) == 0)
        std::printf("[Vita] Free memory: user=%d KiB, CDRAM=%d KiB, contiguous=%d KiB\n",
                    memory.size_user / 1024, memory.size_cdram / 1024, memory.size_phycont / 1024);
    setenv("PIKMIN_ALLOC_TRACE", "1", 0);
    if (!ensure_directory("save") || !ensure_directory("config") || !ensure_directory("cache"))
        return fail("Cannot create save, config or cache directories.");
    // Keep these relative: std::filesystem uses POSIX absolute-path rules,
    // whereas Vita device prefixes (ux0:) are resolved by newlib's I/O layer.
    if (setenv("NECTAR_SAVE_DIR", "save", 1) != 0 || setenv("NECTAR_PORTABLE", "0", 1) != 0)
        return fail("Cannot configure game storage.");

    struct stat st;
    if (stat("assets/dataDir", &st) != 0 || !S_ISDIR(st.st_mode))
        return fail("Missing game data. Copy your extracted Pikmin 1 assets folder to ux0:data/OpenNectar/pikmin1/assets.");
    const bool has_shader_compiler =
        (stat("ur0:data/libshacccg.suprx", &st) == 0 && S_ISREG(st.st_mode))
        || (stat("ur0:data/external/libshacccg.suprx", &st) == 0 && S_ISREG(st.st_mode));
    if (!has_shader_compiler)
        return fail("vitaGL needs libshacccg.suprx installed before this developer build can compile shaders.");
    return true;
}
