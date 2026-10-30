#include "pc_vita_tev_cg.h"
#include "pc_vita_vertex_cg.h"
#include "pc_vita_transform.h"
#include "../gl/pc_uniform_location.h"
#include "../gl/pc_matrix_rows.h"
#include <cassert>
#include <cstdio>
#include <string>

int main() {
    static_assert(pc_uniform_location_valid(-2147483647, true), "Vita encoded location");
    static_assert(pc_uniform_location_valid(2147483647, true), "Vita encoded location");
    static_assert(!pc_uniform_location_valid(-1, true), "missing Vita uniform");
    static_assert(!pc_uniform_location_valid(0, true), "null Vita uniform");
    static_assert(pc_uniform_location_valid(0, false), "desktop location zero");
    static_assert(!pc_uniform_location_valid(-2, false), "invalid desktop location");
    // Exercise signed-register wrapping, comparison combiners, all sampler
    // units, channel 1 and the largest GX material before device compilation.
    for (int op : {0, 1, 8, 9, 10, 11, 12, 13, 14, 15}) {
        PcTevShaderKey key;
        key.numStages = 16;
        key.fog = 1;
        key.alphaComp0 = 3; // normalized reference, inclusive LEQUAL
        key.alphaComp1 = 4; // strictly GREATER
        for (int i = 0; i < 16; ++i) {
            auto& stage = key.stages[i];
            stage.texMap = i % 8;
            stage.texCoord = i % 4;
            stage.rasChannel = i % 2;
            stage.colorOp = op;
            stage.alphaOp = op;
            stage.colorClamp = stage.alphaClamp = 0;
        }
        const auto source = pc_vita_cg::vita_cg_fragment_source(key);
        assert(source.find("TEXUNIT7") != std::string::npos);
        assert(source.find("uTevKonst_15") != std::string::npos);
        assert(source.find("TEXCOORD5") != std::string::npos);
        assert(source.find("prev.a <= uAlphaRef0") != std::string::npos);
        assert(source.find("prev.a > uAlphaRef1") != std::string::npos);
        assert(source.find("q - 256.0 * floor(q / 256.0)") != std::string::npos);
        assert(source.find("out_color = float4(prev.rgb * uOutTint, prev.a)") != std::string::npos);
        assert(source.find("#version") == std::string::npos);
        assert(source.find("gl_FragCoord") == std::string::npos);
        assert(source.find("vec3") == std::string::npos);
    }
    const auto untextured = pc_vita_cg::vita_cg_fragment_source(PcTevShaderKey{});
    assert(untextured.find("sampler2D") == std::string::npos);
    assert(untextured.find("uFogColour") == std::string::npos);
    assert(untextured.find("discard;") == std::string::npos);
    // A translated matrix catches the multiplication-order regression that
    // identity matrices and the untransformed pink triangle could not reveal.
    const float glColumnMajor[16] = {2,0,0,0, 0,3,0,0, 0,0,4,0, 5,6,7,1};
    const float pos[4] = {1,2,3,1};
    float cgRowVector[4] = {};
    for (int c=0; c<4; ++c)
        for (int r=0; r<4; ++r) cgRowVector[c] += pos[r] * glColumnMajor[r*4+c];
    assert(cgRowVector[0] == 7 && cgRowVector[1] == 12 && cgRowVector[2] == 19 && cgRowVector[3] == 1);
    float rows[16];
    pc_matrix4_rows(glColumnMajor, rows);
    for (int r = 0; r < 4; ++r) {
        float result = 0;
        for (int c = 0; c < 4; ++c) result += rows[r * 4 + c] * pos[c];
        assert(result == cgRowVector[r]);
    }
    const float normal[9] = {1,2,3,4,5,6,7,8,9};
    float nrows[12]; pc_matrix3_rows(normal, nrows);
    assert(nrows[0] == 1 && nrows[1] == 4 && nrows[2] == 7 && nrows[3] == 0);
    assert(nrows[8] == 3 && nrows[9] == 6 && nrows[10] == 9 && nrows[11] == 0);
    const std::string vertex = kPcVitaVertexCg;
    assert(vertex.find("out_position = aPos;") != std::string::npos);
    assert(vertex.find("float4 aPos") != std::string::npos);
    assert(vertex.find("uPosPalette") == std::string::npos);
    assert(vertex.find("uProjRows") == std::string::npos);
    // Perspective keeps w separate from the normal-palette slot. Translation
    // must happen exactly once for raw, palette-selected and pretransformed vertices.
    const float projection[16] = {2,0,0,0, 0,3,0,0, 0,0,-1,-1, 0,0,-2,0};
    const float model[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 4,5,-10,1};
    const float input[3] = {1,2,3};
    float view[4], clip[4], alreadyView[4], alreadyClip[4];
    pc_vita_transform(input, model, projection, view, clip);
    assert(view[0] == 5 && view[1] == 7 && view[2] == -7);
    assert(clip[0] == 10 && clip[1] == 21 && clip[2] == 5 && clip[3] == 7);
    pc_vita_transform(view, nullptr, projection, alreadyView, alreadyClip);
    for (int i = 0; i < 4; ++i) assert(alreadyClip[i] == clip[i]);
    const float behind[3] = {1,2,3};
    pc_vita_transform(behind, nullptr, projection, view, clip);
    assert(clip[3] == -3); // No CPU perspective divide or behind-eye clamping.
    const float ortho[16] = {2,0,0,0, 0,3,0,0, 0,0,1,0, -1,-1,0,1};
    pc_vita_transform(input, nullptr, ortho, view, clip);
    assert(clip[0] == 1 && clip[1] == 5 && clip[2] == 3 && clip[3] == 1);
    std::puts("Native Cg generator and matrix contract checks passed");
}
