# PS Vita port: first compilation milestone

**Status: development only. There is no runnable Vita game or playable VPK.**

The first target is Pikmin 1, based on source revision
`dc92d469517aaf899c4a6be849936e274a9c7303`. Pikmin 2 is outside this milestone.

## Implemented

- An explicit VitaSDK-only CMake configuration compiles the actual Pikmin 1
  legacy game sources into `libpikmin_legacy.a`. It does not substitute an empty
  demo for the game. The compiler checks ARM32/newlib portability; an archive
  does not establish that the complete game links or runs.
- `libpikmin_vita_os.a` compiles the OS shim and a Vita-specific readable-memory
  implementation against the real VitaSDK headers. The latter uses
  `sceKernelGetMemBlockInfoByAddr` and read permissions instead of Linux procfs.
  Host tests exercise failed queries, bounds, permissions, and integer overflow.
- The allocator has a Vita path using zeroed real allocations rather than
  `mmap`, which VitaSDK's newlib does not provide. Unlike the desktop path,
  these allocations consume physical memory immediately.
- `PIKMIN_OS_ARENA_MIB` controls the simulated arena. Desktop defaults remain
  256 MiB; Vita defaults to an **experimental 32 MiB**, saving 224 MiB of static
  arena reservation. OS memory-size reports use that same configured arena.
  This does not establish a working total memory budget.
- CI downloads a checksum-pinned VitaSDK and builds both developer archives.
  A normal Vita configuration fails explicitly until a renderer/runtime exists.

## Vita scope

The Vita profile excludes the desktop launcher, Nectar Map Maker and Model Maker.
`PIKMIN_SINGLE_PLAYER_ONLY` defaults to ON for Vita and OFF for desktop. It replaces
local co-op/VS state and alternate captain selection with an Olimar-only module.
The title omits Play Together, and player/captain prompts resolve to one player
and Olimar. Constant-false co-op queries let the compiler remove split-screen
branches in the legacy sources. Pikmin 2 model auto-import is skipped.

These gates are covered by a host test that attempts to enable two players,
VS and Louie anyway. UI layout and controller behavior still require a runnable
game and device testing. This does not yet remove all custom-map/model systems
from shared code, nor does it establish a measured memory saving for them.

## Reproduce

Install VitaSDK and CMake 3.16 or newer, then from the repository root:

```sh
export VITASDK=/absolute/path/to/vitasdk
bash scripts/build-vita-core.sh
```

The validated SDK archive is
`vitasdk-x86_64-linux-gnu-2026-10-15_05-01-16.tar.bz2`, release
`sdk-snapshot-20261015.850.1` of [VitaSDK autobuilds](https://github.com/vitasdk/autobuilds/releases/tag/sdk-snapshot-20261015.850.1).
Its SHA-256 is
`801f483928523fe63547faf58a7dfdaca7482b26c33af0064acea96a906f7662`.
The SDK is an external development dependency and is not committed here.

To override the provisional arena budget:

```sh
bash scripts/build-vita-core.sh -DPIKMIN_OS_ARENA_MIB=48
```

Valid budgets are 24 through 256 MiB. Use a separate `VITA_BUILD_DIR` if keeping
multiple configurations. The script defaults to two parallel compiler processes.

## Validation performed

- VitaSDK GCC 15.2.0 Release cross-compilation completed for all **383 game
  objects** and the **three Vita OS/single-player objects**.
- `arm-vita-eabi-objdump -f` accepted every member of both archives as ARM code.
  The build script now performs this integrity check before reporting success.
- Symbol inspection confirmed a `0x02000000`-byte (32 MiB) simulated arena.
- The host Vita memory-query and single-player tests passed. The existing
  desktop co-op switch test passed with the normal feature configuration.
- The changed settings code passed Vita compiler syntax checks with single-player
  mode on and off; the desktop title feature configuration also passed.
- CMake rejected a non-Vita core build, an unsupported full Vita game build,
  an out-of-range arena, a Vita editor build, and single-player plus netplay.

These are compilation and host checks. They do not validate a linked game,
graphics, actual Vita allocation behavior, menu layout, or gameplay.

## Work needed before a game build

1. **Graphics:** `pc_window.cpp` requests OpenGL 3.3 or GLES 3.0. The renderer
   emits GLSL 140/330 or GLSL ES 300, loads modern GL entry points and uses depth textures,
   framebuffer blits, and vertex-ID-generated geometry. A Vita graphics backend
   needs explicit API and shader compatibility checks. Merely changing the
   version string or linking vitaGL is not validation. Start with one textured
   GX draw at 960x544, then test representative TEV materials and alpha/depth
   behavior before enabling shadows or postprocessing.
2. **Memory:** the PC allocator explicitly relies on demand-paged large heap
   buffers (`sysNew.cpp`), with comments documenting approximately 253 MiB and
   245 MiB overlay/application allocations. With real Vita allocations, reducing
   the arena alone is insufficient. Measure startup and level peaks, change heap
   backing/lifetimes, and bound texture/mesh caches before declaring a viable
   budget. Allocation failure must remain an error; it cannot be hidden by
   silently aliasing unrelated heaps.
3. **Runtime:** integrate the Vita readable-memory implementation into the full
   executable instead of the desktop OS query. Add writable asset/save/settings
   paths, logging, unsupported desktop-feature handling, thread integration,
   and real controller mappings. None of those are proven by archive creation.
4. **Packaging and device validation:** link an ELF, produce a SELF/VPK, then test
   boot, asset loading, saving/reloading, audio, controls, level transitions and
   sustained play on hardware. No frame rate or playability is claimed yet.

The source does not contain extracted game assets. This milestone does not
require or distribute them; a future running game will need the existing asset
extraction workflow and the user's game data.
