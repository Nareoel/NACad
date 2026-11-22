#include "core/ShaderSource.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>

namespace {

constexpr std::string_view cBlanks{" \t"};

std::string_view sTrim(std::string_view line) {
    if (line.ends_with('\r')) {
        line.remove_suffix(1);
    }
    const auto first = line.find_first_not_of(cBlanks);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = line.find_last_not_of(cBlanks);
    return line.substr(first, last - first + 1);
}

bool sIsVersionDirective(std::string_view line) {
    if (!line.starts_with('#')) {
        return false;
    }
    line.remove_prefix(1);
    line.remove_prefix(std::min(line.find_first_not_of(cBlanks), line.size()));
    constexpr std::string_view cVersion{"version"};
    if (!line.starts_with(cVersion)) {
        return false;
    }
    line.remove_prefix(cVersion.size());
    return line.empty() || cBlanks.find(line.front()) != std::string_view::npos;
}

struct VersionLine {
    std::size_t index;  // 0-based line number
    std::size_t begin;  // offset of the line's first character
    std::size_t end;    // offset just past the line's '\n' (or the source size)
};

std::optional<VersionLine> sFindVersionLine(std::string_view source) {
    std::size_t begin = 0;
    for (std::size_t index = 0; begin < source.size(); ++index) {
        const auto newline = std::min(source.find('\n', begin), source.size());
        const auto end = std::min(newline + 1, source.size());
        const auto line = sTrim(source.substr(begin, newline - begin));
        if (!line.empty() && !line.starts_with("//")) {
            if (sIsVersionDirective(line)) {
                return VersionLine{index, begin, end};
            }
            return {};
        }
        begin = end;
    }
    return {};
}

}  // namespace

namespace Viewer {

std::string shaderHeader(ShaderDialect dialect) {
    if (dialect == ShaderDialect::GlslEs300) {
        return "#version 300 es\n"
               "precision highp float;\n"
               "precision highp int;\n"
               "precision highp sampler2DArray;\n";
    }
    return "#version 330 core\n";
}

std::string prepareShaderSource(std::string_view source, ShaderDialect dialect) {
    const auto header = shaderHeader(dialect);
    const auto versionLine = sFindVersionLine(source);
    if (!versionLine) {
        return header + "#line 1\n" + std::string(source);
    }
    std::string result(source.substr(0, versionLine->begin));
    result += header;
    if (std::ranges::count(header, '\n') > 1) {
        result += "#line " + std::to_string(versionLine->index + 2) + "\n";
    }
    result += source.substr(versionLine->end);
    return result;
}

}  // namespace Viewer
