#pragma once
// Convert the port's column-major matrices to explicit Cg dot-product rows.
inline void pc_matrix4_rows(const float* m, float* rows, int count = 4) {
    for (int r = 0; r < count; ++r)
        for (int c = 0; c < 4; ++c) rows[r * 4 + c] = m[c * 4 + r];
}
inline void pc_matrix3_rows(const float* m, float* rows) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) rows[r * 4 + c] = m[c * 3 + r];
        rows[r * 4 + 3] = 0.0f;
    }
}
