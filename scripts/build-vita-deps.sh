#!/usr/bin/env bash
# Installs the pinned vitaGL libraries and builds SDL's Vita/vitaGL backend.
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
: "${VITASDK:?Set VITASDK to a writable VitaSDK installation}"
deps_dir="${VITA_DEPS_DIR:-$repo_root/build/vita-deps}"
mkdir -p "$deps_dir/packages" "$VITASDK/arm-vita-eabi"
while read -r package checksum; do
    archive="$deps_dir/packages/$package.tar.xz"
    if [[ ! -f "$archive" ]]; then
        curl --fail --location --retry 3 \
            "https://github.com/vitasdk/packages/releases/download/master/$package.tar.xz" \
            -o "$archive"
    fi
    echo "$checksum  $archive" | sha256sum --check -
    python3 - "$archive" "$VITASDK/arm-vita-eabi" <<'PY'
import sys, tarfile
with tarfile.open(sys.argv[1]) as archive:
    archive.extractall(sys.argv[2], filter="data")
PY
done <<'PACKAGES'
vitaGL 752bd2974afbcaf471600acd5ca27e480522e3d9bd4f5628c76254ff2a8d8caa
vitaShaRK 704af0538cf0bd8b55af33b69e807e903589244460a6797fb85c7ac8553dfd30
SceShaccCgExt ded53f55e59a39b32cc1167d8a00f46b239e9844eb8a65870d3dba2f30fa88ba
libmathneon a27601332424dd1e1ba595102a1be9095176b592213d30fec37af3c3510248c0
taihen afced213d9d308a665ef91809fd728614a92ec31b2fcf61cf1f6a3e98bec4014
PACKAGES

sdl_revision=f99bb638f7eb256fa812e9da15d4e295a170640c
sdl_source="${VITA_SDL_SOURCE:-$deps_dir/SDL-$sdl_revision}"
sdl_build="${VITA_SDL_BUILD_DIR:-$deps_dir/sdl-build}"
if [[ ! -e "$sdl_source/.git" ]]; then
    git init "$sdl_source"
    git -C "$sdl_source" remote add origin https://github.com/Northfear/SDL.git
    git -C "$sdl_source" fetch --depth 1 origin "$sdl_revision"
    git -C "$sdl_source" checkout --detach FETCH_HEAD
fi
if [[ "$(git -C "$sdl_source" rev-parse HEAD)" != "$sdl_revision" ]]; then
    echo "SDL dependency checkout does not match the pinned revision." >&2
    exit 1
fi
git -C "$sdl_source" diff --quiet
cmake -S "$sdl_source" -B "$sdl_build" \
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS=-pipe -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF \
    -DVIDEO_VITA_VGL=ON -DVIDEO_VITA_PVR=OFF -DVIDEO_VITA_PIB=OFF
cmake --build "$sdl_build" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
"$VITASDK/bin/arm-vita-eabi-objdump" -f "$sdl_build/libSDL2.a" > /dev/null
cmake --install "$sdl_build"
