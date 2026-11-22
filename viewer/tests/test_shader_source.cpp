#include "core/ShaderSource.h"

#include <doctest/doctest.h>
#include <string>

namespace {
using Viewer::prepareShaderSource;
using Viewer::ShaderDialect;
using Viewer::shaderHeader;

const std::string cCoreHeader{"#version 330 core\n"};
const std::string cEsHeader{
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2DArray;\n"};
}  // namespace

TEST_CASE("shader headers are exact") {
    CHECK(shaderHeader(ShaderDialect::Glsl330Core) == cCoreHeader);
    CHECK(shaderHeader(ShaderDialect::GlslEs300) == cEsHeader);
}

TEST_CASE("a desktop shader with #version 330 core comes out unchanged") {
    const std::string source{"#version 330 core\n\nin vec3 aPos;\nvoid main() {}\n"};
    CHECK(prepareShaderSource(source, ShaderDialect::Glsl330Core) == source);
}

TEST_CASE("the ES header replaces #version and restores the line numbers") {
    CHECK(prepareShaderSource("#version 330 core\nx\ny\n", ShaderDialect::GlslEs300) ==
          cEsHeader + "#line 2\nx\ny\n");
}

TEST_CASE("a source without #version gets the header and #line 1") {
    CHECK(prepareShaderSource("x\ny\n", ShaderDialect::Glsl330Core) == cCoreHeader + "#line 1\nx\ny\n");
    CHECK(prepareShaderSource("x\ny\n", ShaderDialect::GlslEs300) == cEsHeader + "#line 1\nx\ny\n");
}

TEST_CASE("leading blank and comment lines stay above the header") {
    CHECK(prepareShaderSource("\n// c\n#version 330 core\nx\n", ShaderDialect::GlslEs300) ==
          "\n// c\n" + cEsHeader + "#line 4\nx\n");
    CHECK(prepareShaderSource(" \t\n  // c\n#version 330 core\nx\n", ShaderDialect::Glsl330Core) ==
          " \t\n  // c\n" + cCoreHeader + "x\n");
}

TEST_CASE("whitespace inside the #version directive is accepted") {
    CHECK(prepareShaderSource("  #  version 330 core  \nx\n", ShaderDialect::GlslEs300) ==
          cEsHeader + "#line 2\nx\n");
    CHECK(prepareShaderSource("\t#\tversion\t330\nx\n", ShaderDialect::Glsl330Core) == cCoreHeader + "x\n");
}

TEST_CASE("CRLF line endings are handled") {
    CHECK(prepareShaderSource("#version 330 core\r\nx\r\n", ShaderDialect::GlslEs300) ==
          cEsHeader + "#line 2\nx\r\n");
    CHECK(prepareShaderSource("\r\n// c\r\n#version 330 core\r\nx\r\n", ShaderDialect::Glsl330Core) ==
          "\r\n// c\r\n" + cCoreHeader + "x\r\n");
}

TEST_CASE("a #version after another directive is not touched") {
    const std::string source{"#define X 1\n#version 330 core\nx\n"};
    CHECK(prepareShaderSource(source, ShaderDialect::GlslEs300) == cEsHeader + "#line 1\n" + source);
}

TEST_CASE("directives that only look like #version are not replaced") {
    CHECK(prepareShaderSource("#versionx 330\nx\n", ShaderDialect::GlslEs300) ==
          cEsHeader + "#line 1\n#versionx 330\nx\n");
    CHECK(prepareShaderSource("#ifdef X\nx\n#endif\n", ShaderDialect::Glsl330Core) ==
          cCoreHeader + "#line 1\n#ifdef X\nx\n#endif\n");
}

TEST_CASE("a bare #version without a number is still the version line") {
    CHECK(prepareShaderSource("#version\nx\n", ShaderDialect::Glsl330Core) == cCoreHeader + "x\n");
}

TEST_CASE("a source without a trailing newline is handled") {
    CHECK(prepareShaderSource("#version 330 core", ShaderDialect::Glsl330Core) == cCoreHeader);
    CHECK(prepareShaderSource("#version 330 core", ShaderDialect::GlslEs300) == cEsHeader + "#line 2\n");
    CHECK(prepareShaderSource("#version 330 core\nx", ShaderDialect::GlslEs300) == cEsHeader + "#line 2\nx");
    CHECK(prepareShaderSource("x", ShaderDialect::Glsl330Core) == cCoreHeader + "#line 1\nx");
}

TEST_CASE("a source of only blank and comment lines gets the header in front") {
    CHECK(prepareShaderSource("\n// c\n", ShaderDialect::GlslEs300) == cEsHeader + "#line 1\n\n// c\n");
}

TEST_CASE("an empty source gets the header and #line 1") {
    CHECK(prepareShaderSource("", ShaderDialect::Glsl330Core) == cCoreHeader + "#line 1\n");
    CHECK(prepareShaderSource("", ShaderDialect::GlslEs300) == cEsHeader + "#line 1\n");
}
