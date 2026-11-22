#pragma once

#include <string>
#include <string_view>

namespace Viewer {

enum class ShaderDialect { Glsl330Core, GlslEs300 };

// the #version line plus, for GLSL ES, the default precisions
std::string shaderHeader(ShaderDialect dialect);

// replaces the source's #version line with shaderHeader(dialect), or puts the header in front when there
// is none; leading blank and // comment lines stay above it, and a #line directive keeps the compiler's
// line numbers matching the file whenever the header changes the line count
std::string prepareShaderSource(std::string_view source, ShaderDialect dialect);

}  // namespace Viewer
