#pragma once
// Native Vita interface adapted from Brendonm17/Open-Nectar-Vita v0.0.1.
// Positions arrive in homogeneous clip space and view space from the CPU.
// The 21-slot normal palette is retained for GPU lighting and texture generation.
// ATTR0..7 match pc_gfx.cpp. Seven interpolators carry final GX raster
// colours, four UV sets and fog; no desktop GLSL translation is involved.
static const char* kPcVitaVertexCg = R"CG(
float3 do_light(float3 N, float3 wp, float4 lp, float4 lc, float4 lk)
{
    float3 Ld = lp.xyz - wp;
    float dist = length(Ld);
    float3 L = Ld / max(dist, 0.001);
    float diff = max(dot(N, L), 0.0);
    if (lk.w > 0.5)
        diff *= saturate(lk.x + lk.y * dist + lk.z * dist * dist);
    return diff * lc.rgb;
}

float3 do_lights(float3 N, float3 wp, int n,
                 float4 lp0, float4 lp1, float4 lp2, float4 lp3,
                 float4 lc0, float4 lc1, float4 lc2, float4 lc3,
                 float4 lk0, float4 lk1, float4 lk2, float4 lk3)
{
    float3 sum = float3(0.0, 0.0, 0.0);
    if (n > 0) sum += do_light(N, wp, lp0, lc0, lk0);
    if (n > 1) sum += do_light(N, wp, lp1, lc1, lk1);
    if (n > 2) sum += do_light(N, wp, lp2, lc2, lk2);
    if (n > 3) sum += do_light(N, wp, lp3, lc3, lk3);
    return sum;
}

float2 pick_tc(int i, float2 t0, float2 t1, float2 t2, float2 t3)
{
    if (i == 0) return t0;
    if (i == 1) return t1;
    if (i == 2) return t2;
    return t3;
}


float2 gen_tc(int mode, float4 M[2], float4 viewPos, float3 nrm, float2 uvIn,
              float2 g0, float2 g1, float2 g2, float2 g3,
              float2 a0, float2 a1, float2 a2, float2 a3)
{
    if (mode == 0) return uvIn;
    float4 src;
    if (mode == 1)       src = viewPos;
    else if (mode == 2)  src = float4(nrm, 1.0);
    else if (mode >= 11) src = float4(pick_tc(mode - 11, g0, g1, g2, g3), 0.0, 1.0);
    else                 src = float4(pick_tc(mode - 3,  a0, a1, a2, a3), 0.0, 1.0);
    return float2(dot(M[0], src), dot(M[1], src));
}

void main(
    float4 aPos       : ATTR0,
    float4 aColor     : ATTR1,
    float3 aNormal    : ATTR2,
    float2 aTexCoord0 : ATTR3,
    float2 aTexCoord1 : ATTR4,
    float2 aTexCoord2 : ATTR5,
    float2 aTexCoord3 : ATTR6,
    float4 aMatrixSlot : ATTR7, // view.xyz and normal-palette slot


    
    

    uniform float4 uNrmPalette[63],

    
    uniform int uNumLights,
    uniform float4 uLightPos_0, uniform float4 uLightPos_1,
    uniform float4 uLightPos_2, uniform float4 uLightPos_3,
    uniform float4 uLightColor_0, uniform float4 uLightColor_1,
    uniform float4 uLightColor_2, uniform float4 uLightColor_3,
    uniform float4 uLightK_0, uniform float4 uLightK_1,
    uniform float4 uLightK_2, uniform float4 uLightK_3,
    uniform float4 uAmbColor,
    uniform int uChan0En,
    uniform float4 uMaterialColor,
    uniform float4 uMaterialColor1,
    uniform int uUseMaterialRgb, uniform int uUseMaterialAlpha, uniform int uUseMaterialRgb1,
    uniform int uChan0AlphaEn, uniform int uChan0AlphaDiff,

    
    uniform int uNumLights1,
    uniform float4 uLightPos1_0, uniform float4 uLightPos1_1,
    uniform float4 uLightPos1_2, uniform float4 uLightPos1_3,
    uniform float4 uLightColor1_0, uniform float4 uLightColor1_1,
    uniform float4 uLightColor1_2, uniform float4 uLightColor1_3,
    uniform float4 uLightK1_0, uniform float4 uLightK1_1,
    uniform float4 uLightK1_2, uniform float4 uLightK1_3,
    uniform float4 uAmbColor1,
    uniform int uChan1En,
    uniform int uChan1AttnFn,
    uniform float4 uSpecHalf1,
    uniform float4 uSpecAttn1,

    
    uniform int uTcMode_0, uniform int uTcMode_1,
    uniform int uTcMode_2, uniform int uTcMode_3,
    uniform float4 uTcRows_0[2], uniform float4 uTcRows_1[2],
    uniform float4 uTcRows_2[2], uniform float4 uTcRows_3[2],

    
    uniform float4 uFogParams,

    out float4 out_position : POSITION,
    out float  vFog      : TEXCOORD6,
    out float4 vColor    : COLOR0,
    out float4 vColor1   : TEXCOORD5,
    out float2 vTexCoord0 : TEXCOORD0,
    out float2 vTexCoord1 : TEXCOORD1,
    out float2 vTexCoord2 : TEXCOORD2,
    out float2 vTexCoord3 : TEXCOORD3
) {
    
    int mi = int(clamp(floor(aMatrixSlot.w + 0.5), 0.0, 20.0)) * 3;
    float4 worldPos = float4(aMatrixSlot.xyz, 1.0);
    out_position = aPos;

    float fogSpan = uFogParams.y - uFogParams.x;
    vFog = abs(fogSpan) < 1e-6 ? 0.0 : (-worldPos.z - uFogParams.x) / fogSpan;
    float3 N = normalize(float3(dot(uNrmPalette[mi].xyz, aNormal), dot(uNrmPalette[mi + 1].xyz, aNormal), dot(uNrmPalette[mi + 2].xyz, aNormal)));


    float3 lit0 = uAmbColor.rgb + do_lights(N, worldPos.xyz, uNumLights,
        uLightPos_0, uLightPos_1, uLightPos_2, uLightPos_3,
        uLightColor_0, uLightColor_1, uLightColor_2, uLightColor_3,
        uLightK_0, uLightK_1, uLightK_2, uLightK_3);

    float3 spec1 = float3(0.0, 0.0, 0.0);
    if (uChan1AttnFn == 0 && uNumLights1 > 0) {
        float cosT = max(dot(N, normalize(uSpecHalf1.xyz)), 0.0);
        float3 quad = float3(1.0, cosT, cosT * cosT);
        float den = dot(uLightK1_0.xyz, quad);
        float att = den > 1e-5 ? saturate(max(0.0, dot(uSpecAttn1.xyz, quad)) / den) : 0.0;
        spec1 = att * uLightColor1_0.rgb;
    }
    float3 diffuse1 = (uChan1AttnFn == 0) ? float3(0.0, 0.0, 0.0)
        : do_lights(N, worldPos.xyz, uNumLights1,
            uLightPos1_0, uLightPos1_1, uLightPos1_2, uLightPos1_3,
            uLightColor1_0, uLightColor1_1, uLightColor1_2, uLightColor1_3,
            uLightK1_0, uLightK1_1, uLightK1_2, uLightK1_3);
    float3 lit1 = uAmbColor1.rgb + diffuse1 + spec1;

    
    float3 lit0s = (uChan0En != 0) ? saturate(lit0) : float3(1.0, 1.0, 1.0);
    float3 lit1s = (uChan1En != 0) ? saturate(lit1) : float3(1.0, 1.0, 1.0);
    float3 base0 = (uUseMaterialRgb != 0) ? uMaterialColor.rgb : aColor.rgb;
    float  alpha0 = (uUseMaterialAlpha != 0) ? uMaterialColor.a : aColor.a;
    float3 base1 = (uUseMaterialRgb1 != 0) ? uMaterialColor1.rgb : aColor.rgb;
    if (uChan0AlphaEn != 0) {
        float alphaLight = uAmbColor.a;
        if (uNumLights > 0) {
            float3 ld = uLightPos_0.xyz - worldPos.xyz;
            float dist = length(ld);
            float diff = uChan0AlphaDiff == 0 ? 1.0 : dot(N, ld / max(dist, 0.001));
            if (uChan0AlphaDiff == 2) diff = max(diff, 0.0);
            if (uLightK_0.w > 0.5) diff *= saturate(uLightK_0.x + uLightK_0.y * dist + uLightK_0.z * dist * dist);
            alphaLight += diff * uLightColor_0.a;
        }
        if (uNumLights > 1) {
            float3 ld = uLightPos_1.xyz - worldPos.xyz;
            float dist = length(ld);
            float diff = uChan0AlphaDiff == 0 ? 1.0 : dot(N, ld / max(dist, 0.001));
            if (uChan0AlphaDiff == 2) diff = max(diff, 0.0);
            if (uLightK_1.w > 0.5) diff *= saturate(uLightK_1.x + uLightK_1.y * dist + uLightK_1.z * dist * dist);
            alphaLight += diff * uLightColor_1.a;
        }
        if (uNumLights > 2) {
            float3 ld = uLightPos_2.xyz - worldPos.xyz;
            float dist = length(ld);
            float diff = uChan0AlphaDiff == 0 ? 1.0 : dot(N, ld / max(dist, 0.001));
            if (uChan0AlphaDiff == 2) diff = max(diff, 0.0);
            if (uLightK_2.w > 0.5) diff *= saturate(uLightK_2.x + uLightK_2.y * dist + uLightK_2.z * dist * dist);
            alphaLight += diff * uLightColor_2.a;
        }
        if (uNumLights > 3) {
            float3 ld = uLightPos_3.xyz - worldPos.xyz;
            float dist = length(ld);
            float diff = uChan0AlphaDiff == 0 ? 1.0 : dot(N, ld / max(dist, 0.001));
            if (uChan0AlphaDiff == 2) diff = max(diff, 0.0);
            if (uLightK_3.w > 0.5) diff *= saturate(uLightK_3.x + uLightK_3.y * dist + uLightK_3.z * dist * dist);
            alphaLight += diff * uLightColor_3.a;
        }
        alpha0 *= saturate(alphaLight);
    }
    vColor  = float4(saturate(base0 * lit0s), alpha0);
    vColor1 = float4(saturate(base1 * lit1s), alpha0);

    float2 z = float2(0.0, 0.0);
    float2 tc0 = gen_tc(uTcMode_0, uTcRows_0, worldPos, aNormal, aTexCoord0, z,   z,   z,   z, aTexCoord0, aTexCoord1, aTexCoord2, aTexCoord3);
    float2 tc1 = gen_tc(uTcMode_1, uTcRows_1, worldPos, aNormal, aTexCoord1, tc0, z,   z,   z, aTexCoord0, aTexCoord1, aTexCoord2, aTexCoord3);
    float2 tc2 = gen_tc(uTcMode_2, uTcRows_2, worldPos, aNormal, aTexCoord2, tc0, tc1, z,   z, aTexCoord0, aTexCoord1, aTexCoord2, aTexCoord3);
    float2 tc3 = gen_tc(uTcMode_3, uTcRows_3, worldPos, aNormal, aTexCoord3, tc0, tc1, tc2, z, aTexCoord0, aTexCoord1, aTexCoord2, aTexCoord3);
    vTexCoord0 = tc0;
    vTexCoord1 = tc1;
    vTexCoord2 = tc2;
    vTexCoord3 = tc3;
}
)CG";
