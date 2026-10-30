/* Cg fragment-shader generator for the PS Vita. */
#pragma once
// Native Cg interface adapted from Brendonm17/Open-Nectar-Vita v0.0.1.
// Combiner arithmetic and normalized alpha references match our GLSL backend.
#include "../gl/pc_tev_shader.h"


#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace pc_vita_cg {
std::string vita_cg_fragment_source(const PcTevShaderKey&, bool);
unsigned vita_tev_needed_texcoords(const PcTevShaderKey&);
namespace {

// Combiner operand ids, named to keep the generator readable.
enum : uint8_t {
	CC_CPREV = 0, CC_APREV, CC_C0, CC_A0, CC_C1, CC_A1, CC_C2, CC_A2,
	CC_TEXC, CC_TEXA, CC_RASC, CC_RASA, CC_ONE, CC_HALF, CC_KONST, CC_ZERO
};
enum : uint8_t {
	CA_APREV = 0, CA_A0, CA_A1, CA_A2, CA_TEXA, CA_RASA, CA_KONST, CA_ZERO
};
enum : uint8_t { CMP_NEVER = 0, CMP_ALWAYS = 7 };

const char* color_arg_expr(uint8_t id)
{
	switch (id) {
	case CC_CPREV: return "prev.rgb";
	case CC_APREV: return "float3(prev.a)";
	case CC_C0:    return "c0.rgb";
	case CC_A0:    return "float3(c0.a)";
	case CC_C1:    return "c1.rgb";
	case CC_A1:    return "float3(c1.a)";
	case CC_C2:    return "c2.rgb";
	case CC_A2:    return "float3(c2.a)";
	case CC_TEXC:  return "tex.rgb";
	case CC_TEXA:  return "float3(tex.a)";
	case CC_RASC:  return "rast.rgb";
	case CC_RASA:  return "float3(rast.a)";
	case CC_ONE:   return "float3(1.0)";
	case CC_HALF:  return "float3(0.5)";
	case CC_KONST: return "konst.rgb";
	default:       return "float3(0.0)";
	}
}

const char* alpha_arg_expr(uint8_t id)
{
	switch (id) {
	case CA_APREV: return "prev.a";
	case CA_A0:    return "c0.a";
	case CA_A1:    return "c1.a";
	case CA_A2:    return "c2.a";
	case CA_TEXA:  return "tex.a";
	case CA_RASA:  return "rast.a";
	case CA_KONST: return "konst.a";
	default:       return "0.0";
	}
}

const char* bias_literal(uint8_t bias)
{
	return bias == 1 ? "0.5" : bias == 2 ? "-0.5" : "0.0";
}

const char* scale_literal(uint8_t scale)
{
	return scale == 1 ? "2.0" : scale == 2 ? "4.0" : scale == 3 ? "0.5" : "1.0";
}

const char* register_name(uint8_t reg)
{
	switch (reg) {
	case 0:  return "prev";
	case 1:  return "c0";
	case 2:  return "c1";
	default: return "c2";
	}
}

const char* swizzle_component(uint8_t component)
{
	switch (component) {
	case 0:  return "r";
	case 1:  return "g";
	case 2:  return "b";
	default: return "a";
	}
}

bool swap_table_is_identity(const uint8_t table[4])
{
	return table[0] == 0 && table[1] == 1 && table[2] == 2 && table[3] == 3;
}

// Applies a swap table by generating a swizzle, which the driver resolves for
// free. The ubershader had to index a uniform array and rebuild a float4
// component by component.
void emit_swap(std::string& out, const char* target, const uint8_t table[4])
{
	if (swap_table_is_identity(table)) return;
	out += "\t";
	out += target;
	out += " = ";
	out += target;
	out += ".";
	for (int i = 0; i < 4; ++i) out += swizzle_component(table[i]);
	out += ";\n";
}

// Reproduces alphaCompare() from the ubershader for a known function. Returns
// an empty string when the result is a compile-time constant.
std::string compare_expr(uint8_t func, const char* value, const char* reference)
{
	char buffer[256];
	switch (func) {
	case 1: snprintf(buffer, sizeof(buffer), "(%s < %s)", value, reference); break;
	case 2: snprintf(buffer, sizeof(buffer), "(abs(%s - %s) < (0.5 / 255.0))", value, reference); break;
	case 3: snprintf(buffer, sizeof(buffer), "(%s <= %s)", value, reference); break;
	case 4: snprintf(buffer, sizeof(buffer), "(%s > %s)", value, reference); break;
	case 5: snprintf(buffer, sizeof(buffer), "(abs(%s - %s) >= (0.5 / 255.0))", value, reference); break;
	case 6: snprintf(buffer, sizeof(buffer), "(%s >= %s)", value, reference); break;
	default: return std::string();
	}
	return std::string(buffer);
}

bool compare_is_static(uint8_t func) { return func == CMP_NEVER || func == CMP_ALWAYS; }

bool combine_static(uint8_t op, bool test0, bool test1)
{
	switch (op) {
	case 0:  return test0 && test1;
	case 1:  return test0 || test1;
	case 2:  return test0 != test1;
	default: return test0 == test1;
	}
}

void emit_stage(std::string& out, const PcTevShaderKey& key, int index)
{
	const PcTevStageKey& stage = key.stages[index];
	char line[512];

	snprintf(line, sizeof(line), "\t// stage %d\n\t{\n", index);
	out += line;

	// Exactly one texture fetch, against the one sampler this stage names.
	if (stage.texMap >= 0) {
		snprintf(line, sizeof(line), "\t\tfloat4 tex = tex2D(uTex%d, vTexCoord%d);\n",
		         int(stage.texMap), int(stage.texCoord));
	} else {
		snprintf(line, sizeof(line), "\t\tfloat4 tex = float4(1.0);\n");
	}
	out += line;
	std::string swap;
	emit_swap(swap, "tex", key.swapTable[stage.texSwapSel & 3]);
	if (!swap.empty()) out += "\t" + swap;

	if (stage.rasChannel < 0) {
		out += "\t\tfloat4 rast = float4(0.0);\n";
	} else if (stage.rasChannel == 1) {
		out += "\t\tfloat4 rast = rast1;\n";
	} else {
		out += "\t\tfloat4 rast = rast0;\n";
	}
	swap.clear();
	emit_swap(swap, "rast", key.swapTable[stage.rasSwapSel & 3]);
	if (!swap.empty()) out += "\t" + swap;

	// Indexed with a literal, so this stays a compile-time selection. The name
	// deliberately matches the ubershader's uniform so both programs are fed by
	// the same upload path.
	snprintf(line, sizeof(line), "\t\tfloat4 konst = uTevKonst_%d;\n", index);
	out += line;

	// ── colour ──
	const std::string caW = std::string("tevU8(") + color_arg_expr(stage.colorIn[0]) + ")";
	const std::string cbW = std::string("tevU8(") + color_arg_expr(stage.colorIn[1]) + ")";
	const std::string ccW = std::string("tevU8(") + color_arg_expr(stage.colorIn[2]) + ")";
	const char* ca = caW.c_str();
	const char* cb = cbW.c_str();
	const char* cc = ccW.c_str();
	const char* cd = color_arg_expr(stage.colorIn[3]);

	if (stage.colorOp >= 8) {
		// Comparison operations, ported from compareTevColor() with the op
		// resolved, so only the taken branch is generated.
		const bool isEqual = (stage.colorOp & 1) != 0;
		if (stage.colorOp >= 14) {
			snprintf(line, sizeof(line),
			         "\t\tfloat3 qa = floor(clamp(%s, 0.0, 1.0) * 255.0 + 0.5);\n"
			         "\t\tfloat3 qb = floor(clamp(%s, 0.0, 1.0) * 255.0 + 0.5);\n"
			         "\t\tbool3 cpass = (qa %s qb);\n"
			         "\t\tfloat3 cResult = %s + float3(cpass.x ? (%s).r : 0.0, cpass.y ? (%s).g : 0.0, cpass.z ? (%s).b : 0.0);\n",
			         ca, cb, isEqual ? "==" : ">", cd, cc, cc, cc);
			out += line;
		} else {
			// packTevCompare(), with the component count resolved.
			const int components = (stage.colorOp < 10) ? 1 : (stage.colorOp < 12) ? 2 : 3;
			auto pack = [components](const char* name) {
				std::string expr(name);
				expr += ".r";
				if (components >= 2) { expr += " + "; expr += name; expr += ".g * 256.0"; }
				if (components >= 3) { expr += " + "; expr += name; expr += ".b * 65536.0"; }
				return expr;
			};
			snprintf(line, sizeof(line),
			         "\t\tfloat3 qa = floor(clamp(%s, 0.0, 1.0) * 255.0 + 0.5);\n"
			         "\t\tfloat3 qb = floor(clamp(%s, 0.0, 1.0) * 255.0 + 0.5);\n",
			         ca, cb);
			out += line;
			out += "\t\tfloat av = " + pack("qa") + ";\n";
			out += "\t\tfloat bv = " + pack("qb") + ";\n";
			snprintf(line, sizeof(line),
			         "\t\tbool cpass = %s;\n"
			         "\t\tfloat3 cResult = %s + (cpass ? %s : float3(0.0));\n",
			         isEqual ? "(av == bv)" : "(av > bv)", cd, cc);
			out += line;
		}
	} else {
		snprintf(line, sizeof(line), "\t\tfloat3 cMix = %s * (float3(1.0) - %s) + %s * %s;\n",
		         ca, cc, cb, cc);
		out += line;
		snprintf(line, sizeof(line), "\t\tfloat3 cResult = %s %c cMix;\n",
		         cd, stage.colorOp == 0 ? '+' : '-');
		out += line;
		if (stage.colorBias != 0 || stage.colorScale != 0) {
			snprintf(line, sizeof(line), "\t\tcResult = (cResult + %s) * %s;\n",
			         bias_literal(stage.colorBias), scale_literal(stage.colorScale));
			out += line;
		}
	}
	snprintf(line, sizeof(line), "\t\tcResult = clamp(cResult, %s);\n",
	         stage.colorClamp ? "0.0, 1.0" : "-4.0, 4.0");
	out += line;

	// ── alpha ──
	const std::string aaW = std::string("tevU8(") + alpha_arg_expr(stage.alphaIn[0]) + ")";
	const std::string abW = std::string("tevU8(") + alpha_arg_expr(stage.alphaIn[1]) + ")";
	const std::string acW = std::string("tevU8(") + alpha_arg_expr(stage.alphaIn[2]) + ")";
	const char* aa = aaW.c_str();
	const char* ab = abW.c_str();
	const char* ac = acW.c_str();
	const char* ad = alpha_arg_expr(stage.alphaIn[3]);

	if (stage.alphaOp >= 14) {
		const bool isEqual = (stage.alphaOp & 1) != 0;
		snprintf(line, sizeof(line),
		         "\t\tfloat aqa = floor(clamp(%s, 0.0, 1.0) * 255.0 + 0.5);\n"
		         "\t\tfloat aqb = floor(clamp(%s, 0.0, 1.0) * 255.0 + 0.5);\n"
		         "\t\tfloat aResult = %s + (%s ? %s : 0.0);\n",
		         aa, ab, ad, isEqual ? "(aqa == aqb)" : "(aqa > aqb)", ac);
		out += line;
	} else {
		snprintf(line, sizeof(line), "\t\tfloat aMix = %s * (1.0 - %s) + %s * %s;\n",
		         aa, ac, ab, ac);
		out += line;
		// The ubershader routes alpha ops 8..13 into its subtract branch. That
		// is reproduced exactly: this generator must not change pixels.
		snprintf(line, sizeof(line), "\t\tfloat aResult = %s %c aMix;\n",
		         ad, stage.alphaOp == 0 ? '+' : '-');
		out += line;
		if (stage.alphaOp < 8 && (stage.alphaBias != 0 || stage.alphaScale != 0)) {
			snprintf(line, sizeof(line), "\t\taResult = (aResult + %s) * %s;\n",
			         bias_literal(stage.alphaBias), scale_literal(stage.alphaScale));
			out += line;
		}
	}
	snprintf(line, sizeof(line), "\t\taResult = clamp(aResult, %s);\n",
	         stage.alphaClamp ? "0.0, 1.0" : "-4.0, 4.0");
	out += line;

	// GX routes the colour and alpha outputs independently, so each writes
	// only its own components of the destination register.
	snprintf(line, sizeof(line), "\t\t%s.rgb = cResult;\n\t\t%s.a = aResult;\n\t}\n",
	         register_name(stage.colorOutReg), register_name(stage.alphaOutReg));
	out += line;
}


} // namespace

const char* vita_cg_vertex_source(void)
{
	// Vertex Cg hook.
	return nullptr;
}

std::string vita_cg_fragment_source(const PcTevShaderKey& key) { return vita_cg_fragment_source(key, false); }

bool vita_tev_has_alpha_test(const PcTevShaderKey& key) { return !(key.alphaComp0 == CMP_ALWAYS && key.alphaComp1 == CMP_ALWAYS); }

std::string vita_cg_fragment_source(const PcTevShaderKey& key, bool softAlpha)
{
	const int stageCount = key.numStages < 1 ? 1 : (key.numStages > 16 ? 16 : key.numStages);

	bool usesSampler[8] = {};
	bool usesChannel1 = false;
	for (int i = 0; i < stageCount; ++i) {
		const PcTevStageKey& stage = key.stages[i];
		if (stage.texMap >= 0 && stage.texMap < 8) usesSampler[stage.texMap] = true;
		if (stage.rasChannel == 1) usesChannel1 = true;
	}
	// Register-1 fold.
	if (key.useMaterialRgb1 == 2) usesChannel1 = true;

	const bool needsRef0 = !compare_is_static(key.alphaComp0);
	const bool needsRef1 = !compare_is_static(key.alphaComp1);

	std::string out;
	out.reserve(4096);
	char line[256];

	out += "// Generated Cg TEV fragment shader (Open Nectar / PS Vita)\n";

	out += "float tevU8(float v) { float q = floor(v * 255.0 + 0.5); return (q - 256.0 * floor(q / 256.0)) / 255.0; }\n";
    out += "float3 tevU8(float3 v) { float3 q = floor(v * 255.0 + 0.5); return (q - 256.0 * floor(q / 256.0)) / 255.0; }\n";
    out += "void main(\n";
	// Rasterised colours.
	out += "\tfloat4 vColor : COLOR0,\n";
	if (usesChannel1) out += "\tfloat4 vColor1 : TEXCOORD5,\n";
	{
		// Texcoords read by textured stages.
		const unsigned tcMask = vita_tev_needed_texcoords(key);
		char tcl[64];
		for (int t = 0; t < 4; ++t) {
			if (tcMask & (1u << t)) {
				snprintf(tcl, sizeof(tcl), "\tfloat2 vTexCoord%d : TEXCOORD%d,\n", t, t);
				out += tcl;
			}
		}
	}
	for (int i = 0; i < 8; ++i) {
		if (!usesSampler[i]) continue;
		snprintf(line, sizeof(line), "\tuniform sampler2D uTex%d : TEXUNIT%d,\n", i, i);
		out += line;
	}
	out += "\tuniform float4 uTevPrev,\n";
	out += "\tuniform float4 uTevReg0,\n";
	out += "\tuniform float4 uTevReg1,\n";
	out += "\tuniform float4 uTevReg2,\n";
	for (int i = 0; i < stageCount; ++i) {
		snprintf(line, sizeof(line), "\tuniform float4 uTevKonst_%d,\n", i);
		out += line;
	}
	if (needsRef0) out += "\tuniform float uAlphaRef0,\n";
	if (needsRef1) out += "\tuniform float uAlphaRef1,\n";
	// Fog uniform.
	out += "\tuniform float3 uOutTint,\n";
	if (key.fog) out += "\tuniform float4 uFogColour,\n";
	out += "\tfloat vFog : TEXCOORD6,\n";
	out += "\tout float4 out_color : COLOR)\n{\n";

	// Raster channels.
	out += "\tfloat4 rast0 = vColor;\n";
	if (usesChannel1) out += "\tfloat4 rast1 = vColor1;\n";

	out += "\tfloat4 prev = (float4)uTevPrev;\n";
	out += "\tfloat4 c0 = (float4)uTevReg0;\n";
	out += key.useMaterialRgb1 == 2 ? "\tfloat4 c1 = rast1;\n" : "\tfloat4 c1 = (float4)uTevReg1;\n";
	out += "\tfloat4 c2 = (float4)uTevReg2;\n";

	for (int i = 0; i < stageCount; ++i) emit_stage(out, key, i);

	// Fog after the stages, colour only.
	static const char kFogLine[] =
		"\tif (uFogColour.a > 0.0) prev.rgb = lerp(prev.rgb, uFogColour.rgb, (float)saturate(vFog));\n";
	// Soft variant: fog before the zeroing.
	if (key.fog && softAlpha) out += kFogLine;

	if (compare_is_static(key.alphaComp0) && compare_is_static(key.alphaComp1)) {
		const bool passes = combine_static(key.alphaTestOp,
		                                   key.alphaComp0 == CMP_ALWAYS,
		                                   key.alphaComp1 == CMP_ALWAYS);
		if (!passes) out += softAlpha ? "\tprev = float4(0.0, 0.0, 0.0, 0.0);\n" : "\tdiscard;\n";
	} else {
		// 8-bit compare as an alpha threshold.
		std::string test0 = compare_expr(key.alphaComp0, "prev.a", "uAlphaRef0");
		std::string test1 = compare_expr(key.alphaComp1, "prev.a", "uAlphaRef1");
		if (test0.empty()) test0 = (key.alphaComp0 == CMP_ALWAYS) ? "true" : "false";
		if (test1.empty()) test1 = (key.alphaComp1 == CMP_ALWAYS) ? "true" : "false";
		const char* combiner = key.alphaTestOp == 0 ? "&&"
		                     : key.alphaTestOp == 1 ? "||"
		                     : key.alphaTestOp == 2 ? "!=" : "==";
		out += "\tif (!(" + test0 + " " + combiner + " " + test1 + ")) "
		     + (softAlpha ? "prev = float4(0.0, 0.0, 0.0, 0.0);" : "discard;") + "\n";
	}

	if (key.fog && !softAlpha) out += kFogLine;
	out += "\tout_color = float4(prev.rgb * uOutTint, prev.a);\n";
	out += "}\n";
	return out;
}

// Varyings a configuration reads.

unsigned vita_tev_needed_texcoords(const PcTevShaderKey& key)
{
	const int stageCount = key.numStages < 1 ? 1 : (key.numStages > 16 ? 16 : key.numStages);
	unsigned mask = 0;
	for (int i = 0; i < stageCount; ++i) {
		const PcTevStageKey& st = key.stages[i];
		if (st.texMap >= 0) mask |= 1u << (st.texCoord & 3);
	}
	return mask;
}

bool vita_tev_uses_channel1(const PcTevShaderKey& key)
{
	const int stageCount = key.numStages < 1 ? 1 : (key.numStages > 16 ? 16 : key.numStages);
	for (int i = 0; i < stageCount; ++i) {
		if (key.stages[i].rasChannel == 1) return true;
	}
	return key.useMaterialRgb1 == 2;
}

} // namespace pc_vita_cg
