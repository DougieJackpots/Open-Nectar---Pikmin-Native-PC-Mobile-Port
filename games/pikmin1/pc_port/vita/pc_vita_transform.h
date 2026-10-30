#pragma once

// Column-major matrices, matching the GX decoder's CPU transform path.
// Keep homogeneous clip coordinates: dividing here would break interpolation
// and clipping for triangles crossing the near plane.
inline void pc_vita_transform(const float position[3], const float* modelView,
                              const float projection[16], float view[4], float clip[4]) {
    for (int r = 0; r < 3; ++r)
        view[r] = modelView ? modelView[r] * position[0] + modelView[4 + r] * position[1]
                             + modelView[8 + r] * position[2] + modelView[12 + r]
                           : position[r];
    view[3] = 1.0f;
    for (int r = 0; r < 4; ++r)
        clip[r] = projection[r] * view[0] + projection[4 + r] * view[1]
                + projection[8 + r] * view[2] + projection[12 + r];
}
