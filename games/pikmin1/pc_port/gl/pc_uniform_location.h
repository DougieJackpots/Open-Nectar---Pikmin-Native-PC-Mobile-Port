#pragma once
// Desktop locations are indices; Vita locations are encoded pointers.
// Both signs may be valid on Vita. Only its two sentinel values are absent.
constexpr bool pc_uniform_location_valid(int location, bool vita) {
    return vita ? location != -1 && location != 0 : location >= 0;
}
