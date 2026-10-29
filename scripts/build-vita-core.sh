#!/usr/bin/env bash
# Developer milestone only: produces ARM archives, not a game/VPK.
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
: "${VITASDK:?Set VITASDK to a VitaSDK installation}"
if [[ ! -f "$VITASDK/share/vita.toolchain.cmake" ]]; then
    echo "VitaSDK toolchain is missing: $VITASDK/share/vita.toolchain.cmake" >&2
    exit 1
fi
build_dir="${VITA_BUILD_DIR:-$repo_root/build-vita-core}"
cmake -S "$repo_root/games/pikmin1" -B "$build_dir" \
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DPIKMIN_VITA_CORE_ONLY=ON "$@"
cmake --build "$build_dir" --target pikmin_legacy pikmin_vita_os \
    --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
# An archive can be created even if an interrupted compiler left an empty
# member behind. Inspect all members before reporting a successful milestone.
for archive in libpikmin_legacy.a libpikmin_vita_os.a; do
    "$VITASDK/bin/arm-vita-eabi-objdump" -f "$build_dir/$archive" > /dev/null
done
echo "Built Vita game-logic and OS archives. No executable or VPK is produced."
