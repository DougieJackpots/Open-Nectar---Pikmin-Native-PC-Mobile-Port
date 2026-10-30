# PS Vita: experimental Pikmin 1 runtime

**A developer executable and VPK now build successfully. Booting, rendering,
audio, controls, saving and gameplay have NOT been validated on Vita hardware.**
This is not a playable-release announcement. Pikmin 2 is outside this target.
The initial source base was `dc92d469517aaf899c4a6be849936e274a9c7303`.

## Current implementation

- The actual Pikmin 1 game logic, GX renderer, audio mixer and native runtime
  cross-compile and link into an ARM32 executable. VitaSDK converts it to a
  SELF and packages `pikmin1-vita-experimental.vpk` (application `NECTR0001`,
  version `00.01`, displayed as **Nectar Vita Dev**).
- SDL's pinned Vita/vitaGL backend supplies the context and GL function lookup.
  The existing GX-to-GL renderer links against vitaGL. This does not prove that
  its GLSL programs compile or render correctly on the device.
- Vita is locked to the original 30 FPS mode (33.33 ms per game tick), including
  title/menu sections. Imported 60/120/monitor settings are ignored and the FPS
  selector stays at 30. A 60 Hz display report is not a measured game FPS.
  Sustained 30 FPS still requires device performance testing.
- The Vita window defaults to 960x544. Initial streaming VBO allocation is
  4 MiB and the resident mesh arena is 8 MiB, versus desktop defaults of
  16 MiB and 128 MiB. Streaming can still grow for a large draw; these numbers
  are not a measured or enforced total GPU-memory limit.
- Game data, saves, configuration, cache and an append-only `pikmin.log` live
  beneath `ux0:data/OpenNectar/pikmin1`. Startup checks for extracted assets
  and the vitaGL shader compiler before entering the game.
- The C/C++ heap has a provisional 128 MiB budget. The simulated GameCube arena
  defaults to an experimental 32 MiB on Vita and remains 256 MiB on desktop.
  OS memory-size reports match that arena. Large allocations use zeroed real
  memory, not desktop demand-paged `mmap`. Device peak memory is still unknown.
- Readable-memory bounds use `sceKernelGetMemBlockInfoByAddr`, rather than
  procfs. Save/config routing avoids treating a Vita device prefix as a POSIX
  absolute path.
- The Vita profile excludes the desktop launcher, Map Maker and Model Maker.
  Local co-op/VS and alternate captain selection are disabled; Olimar is the
  only captain. Discord, LiveSplit networking and desktop file pickers are
  unavailable. Local speedrun timing is retained.

## Important graphics limitation

vitaGL does not supply `glLogicOp`. The experimental renderer uses the same
fallback as the Android port: GX COPY is represented by blending disabled;
other logic operations currently fall back to COPY and emit a log warning.
Those operations need proper emulation if game content depends on them.
No visual correctness, frame rate or sustained-play claim is made.

## Build

Use Linux with Git, curl, CMake, a C/C++ build tool, Python 3.12 or a Python
version providing `tarfile`'s `filter="data"`, and a writable VitaSDK installation.
The validated SDK is VitaSDK GCC 15.2.0 from
[snapshot sdk-snapshot-20261015.850.1](https://github.com/vitasdk/autobuilds/releases/tag/sdk-snapshot-20261015.850.1):

- Archive: `vitasdk-x86_64-linux-gnu-2026-10-15_05-01-16.tar.bz2`
- SHA-256: `801f483928523fe63547faf58a7dfdaca7482b26c33af0064acea96a906f7662`

From the repository root:

```sh
export VITASDK=/absolute/path/to/vitasdk
bash scripts/build-vita-deps.sh
bash scripts/build-vita-game.sh
```

The dependency script installs checksum-pinned VitaSDK packages for vitaGL,
vitaShaRK, SceShaccCgExt, mathneon and taiHEN, then builds
[Northfear/SDL](https://github.com/Northfear/SDL/tree/vitagl) at commit
`f99bb638f7eb256fa812e9da15d4e295a170640c`, with vitaGL enabled and PVR/PIB off.
The game configuration checks that this SDL backend is present. It corrects
static shader/compiler-library ordering in the imported SDL target.

The result is `build-vita-game/pikmin1-vita-experimental.vpk`. The full target
requires explicit `PIKMIN_VITA_EXPERIMENTAL_GAME=ON`; the script supplies it.
`CMAKE_BUILD_PARALLEL_LEVEL` defaults to two compiler processes. `VITA_BUILD_DIR`
can select a separate output directory. The Actions workflow has a manual
`build_game` option for an experimental package after the core checks.

The lighter compilation-only milestone remains available:

```sh
bash scripts/build-vita-core.sh
```

It produces archives, not a VPK. Arena budgets can be selected using
`-DPIKMIN_OS_ARENA_MIB=48` (valid canonical integers: 24 through 256).
`PIKMIN_SINGLE_PLAYER_ONLY` defaults to ON for Vita and OFF for desktop.

## First device test

1. Use a homebrew-capable Vita with vitaGL's required `libshacccg.suprx` installed.
   See the [vitaGL prerequisites](https://github.com/Rinnegatamante/vitaGL#prerequisites).
   That system module is not distributed in this repository or package.
2. Install the developer VPK using VitaShell.
3. Copy the complete extracted Pikmin 1 `assets` folder into
   `ux0:data/OpenNectar/pikmin1/assets`. The executable defaults to USA Rev 1
   (`GPIE01_01`); use matching data. Other regional builds require selecting
   `PIKMIN_GAME_VERSION` when compiling, as in the upstream port.
4. Launch **Nectar Vita Dev**. Record whether the title screen appears and
   whether it can enter The Impact Site. No game assets are bundled.
5. Retrieve `ux0:data/OpenNectar/pikmin1/pikmin.log`. It records startup, available
   memory, allocation diagnostics, and shader/renderer failures. If it stops
   before creating a log, record the system error code instead.

Provisional built-in controls: left stick moves Olimar, right stick swarms;
Cross = A/throw, Circle = B/whistle, Square = X/disband, Triangle = Y/type
selection; L faces forward, R changes camera height, D-pad Up zooms, Start
pauses, and Select opens settings. These mappings need device verification.

## Validation completed

- Full Release game executable linked as an ARM ELF32 executable with no
  unresolved link errors. VitaSDK SELF conversion and VPK packaging succeeded.
- All 383 legacy archive members passed ARM object inspection. The build script
  enforces archive and executable inspection before reporting success.
- The VPK passed ZIP CRC checks and contains only `eboot.bin` and
  `sce_sys/param.sfo`; SELF magic and title/version metadata were checked.
- The dependency recipe completed using the pinned checkout and cached,
  checksum-verified packages; the game build script completed afterward.
- Host `nectar_paths_test` passed after adding Vita paths (its read-only check
  is skipped when running as root). Earlier memory-query, single-player and
  desktop co-op-switch tests passed in the first milestone.
- The renderer and startup code passed Vita compiler syntax checks.

**Still required:** actual shader compilation on the device, renderer API and
visual correctness checks, peak-memory measurements, controller testing,
audio, save/reload, level transitions and sustained gameplay. A generated VPK
alone is not evidence that any of those work.

## First device log

The first attached log confirms that storage, assets, SDL initialization and the
Vita GPU/context are reached. It ends after shader binary-cache initialization,
before the game loop. It therefore does not establish a gameplay frame rate or
identify the failing shader call. Startup markers now bracket fragment shader
creation/upload/compile, status queries and TEV program linking to narrow down
the next device test. The earlier 60 Hz line describes display refresh only.

The device report also describes the vitaGL logo freezing partway through.
This is consistent with the last logged stage but does not prove which call
hung or crashed. The next developer build avoids compiling the generic dynamic
16-stage TEV evaluator at startup. It uses a small one-stage bootstrap program
and compiles the exact material shaders as needed. All supported TEV stage
counts remain available. A material compile failure stops with diagnostics
rather than using the bootstrap shader to render incorrect materials. This is
a targeted mitigation for the suspected compiler bottleneck, not a confirmed
fix; vertex compilation, the translator or another startup call may still fail.

The attached device crash report shows a main-thread data abort while the
vitaGL splash thread waits. The supplied renderer change disables the application
program-binary cache on Vita: its cache lookup binds vertex attributes before
attaching a vertex shader, which dereferences a null shader in vitaGL. Both the
startup and material cache lookup paths are skipped; vitaGL internal caching is
unaffected. This fixes an unsafe call order, but the device crash is not yet
confirmed resolved. The dump PC and displayed instruction addresses disagree,
so its function labels alone do not establish the failing instruction.

## Current shader diagnostic build

The device log confirms that the ES 1.00 bootstrap fragment passes compilation,
while the shared vertex shader fails with an empty compiler log. Vita uses
immediate compilation with GLOBAL semantics, an ES 1.00 text adapter, bounded
lighting loops, and disabled post-effects/dim/sprite overlays. Startup runs
small compiler probes plus the real vertex shader and a float-clamp variant.
Probe results are flushed to pikmin.log; passing text tests does not establish
device compiler compatibility. The real submitted shader still contains its
integer clamp so the probe comparison can isolate that failure. This is a
diagnostic package, not a confirmed boot or 30 FPS gameplay build.

Pushing vita/shader-diagnostics starts the core and experimental-package jobs.
Download pikmin1-vita-experimental-unvalidated from the completed workflow run.
