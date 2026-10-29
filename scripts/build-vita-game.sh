#!/usr/bin/env bash
# Experimental developer package. Compilation does not establish playability.
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
: "${VITASDK:?Set VITASDK after running scripts/build-vita-deps.sh}"
build_dir="${VITA_BUILD_DIR:-$repo_root/build-vita-game}"
cmake -S "$repo_root/games/pikmin1" -B "$build_dir" \
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DPIKMIN_VITA_CORE_ONLY=OFF -DPIKMIN_VITA_EXPERIMENTAL_GAME=ON \
    -DPIKMIN_ENABLE_IPO=OFF -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "$build_dir" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
"$VITASDK/bin/arm-vita-eabi-objdump" -f "$build_dir/libpikmin_legacy.a" > /dev/null
"$VITASDK/bin/arm-vita-eabi-objdump" -f "$build_dir/bin/nectar" > /dev/null
test -s "$build_dir/pikmin1-vita-experimental.vpk"
echo "Experimental package: $build_dir/pikmin1-vita-experimental.vpk"
echo "Hardware rendering, memory use, saves and gameplay are not validated."
