#pragma once
#include <regex>
#include <string>

// Adapter for the generated GX shaders, not a general GLSL transpiler.
// In particular, samplerCube/textureLod and integer shader features need
// separate handling before any additional shader families use this helper.
inline std::string pc_vita_glsl(std::string source, bool vertex) {
    source = std::regex_replace(source, std::regex(R"(#version[^\n]*\n)"), "");
    source = std::regex_replace(source, std::regex(R"(layout\s*\([^)]*\)\s*)"), "");
    // Restrict interface conversion to line-leading declarations, preserving
    // function parameter qualifiers such as `in vec4 argument`.
    const auto lines = std::regex_constants::ECMAScript | std::regex_constants::multiline;
    source = std::regex_replace(source, std::regex(R"(^[ \t]*in\s+)", lines), vertex ? "attribute " : "varying ");
    if (vertex) {
        source = std::regex_replace(source, std::regex(R"(^[ \t]*out\s+)", lines), "varying ");
    } else {
        source = std::regex_replace(source, std::regex(R"(^[ \t]*out\s+vec4\s+fragColor\s*;[ \t]*\n?)", lines), "");
        source = std::regex_replace(source, std::regex(R"(\bfragColor\b)"), "gl_FragColor");
    }
    source = std::regex_replace(source, std::regex(R"(\btexture\s*\()"), "texture2D(");
    source = std::regex_replace(source, std::regex(R"(for \(int i = 0; i < n; i\+\+\) \{)"), "for (int i = 0; i < 4; i++) { if (i >= n) break;");
    source = std::regex_replace(source, std::regex(R"(for \(int i = 0; i < uNumLights; i\+\+\) \{)"), "for (int i = 0; i < 4; i++) { if (i >= uNumLights) break;");
    return "#version 100\nprecision highp float;\nprecision highp int;\n" + source;
}
