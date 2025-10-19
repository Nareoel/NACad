#include "ShaderProgram.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
std::string sGetFileContent(const std::filesystem::path& filePath) {
    std::ifstream fileStream(filePath);
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    // if (!buffer.str().empty()) {
    //     std::cout << buffer.str() << std::endl;
    // }
    return buffer.str();
}
std::optional<unsigned int> sCreateAndCompileShader(const std::string& shaderSrc, bool isVertexShader) {
    if (shaderSrc.empty()) {
        std::string shaderType = isVertexShader ? "Vertex" : "Fragment";
        std::cout << "Error::Shader::" << shaderType << "::Empty Source\n" << std::endl;
        return {};
    }

    auto shaderId = glCreateShader(isVertexShader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    auto shaderSrcPointer = shaderSrc.c_str();
    glShaderSource(shaderId, 1, &(shaderSrcPointer), NULL);
    glCompileShader(shaderId);

    int shaderCompSuccess{-1};
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &shaderCompSuccess);
    if (!shaderCompSuccess) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
        std::string shaderType = isVertexShader ? "Vertex" : "Fragment";
        std::cout << "Error::Shader::" << shaderType << "::Compilation Failed\n" << infoLog << std::endl;
        return {};
    }
    return shaderId;
}

void sCleanUpShaders(const std::vector<unsigned int>& shadersIds) {
    for (auto& shaderId : shadersIds) {
        glDeleteShader(shaderId);
    }
}

std::optional<unsigned int> sCreateAndLinkProgram(unsigned int vShaderId, unsigned int sShaderId) {
    auto programId = glCreateProgram();
    glAttachShader(programId, vShaderId);
    glAttachShader(programId, sShaderId);
    glLinkProgram(programId);

    int linkSuccess{-1};
    glGetProgramiv(programId, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        char infoLog[512];
        glGetProgramInfoLog(programId, 512, NULL, infoLog);
        std::cout << "Error::Program::Linkage Failed\n" << infoLog << std::endl;
        return {};
    }
    return programId;
}
}  // namespace

std::optional<ShaderProgram> ShaderProgram::createShaderProgram(const std::filesystem::path& vShaderPath,
                                                                const std::filesystem::path& sShaderPath) {
    namespace fs = std::filesystem;
    if (!fs::exists(vShaderPath) || fs::is_directory(vShaderPath)) {
        std::cout << "Can't find shader file: " << vShaderPath << std::endl;
        return {};
    }
    if (!fs::exists(sShaderPath) || fs::is_directory(sShaderPath)) {
        std::cout << "Can't find shader file: " << sShaderPath << std::endl;
        return {};
    }
    auto vShaderStr = sGetFileContent(vShaderPath);
    auto sShaderStr = sGetFileContent(sShaderPath);

    auto vShaderId = sCreateAndCompileShader(vShaderStr, true);
    if (!vShaderId) {
        return {};
    }
    auto sShaderId = sCreateAndCompileShader(sShaderStr, false);
    if (!sShaderId) {
        sCleanUpShaders({*vShaderId});
        return {};
    }

    auto programId = sCreateAndLinkProgram(*vShaderId, *sShaderId);
    sCleanUpShaders({*vShaderId, *sShaderId});
    if (!programId) {
        return {};
    }

    return ShaderProgram(*programId);
}

ShaderProgram::ShaderProgram(unsigned int programId) : programId_{programId} {}

void ShaderProgram::use() { glUseProgram(programId_); }

void ShaderProgram::setUniform(const std::string& varName, bool value) {
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1i(location, value);
}
void ShaderProgram::setUniform(const std::string& varName, int value) {
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1i(location, value);
}
void ShaderProgram::setUniform(const std::string& varName, unsigned int value) {
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1i(location, value);
}
void ShaderProgram::setUniform(const std::string& varName, float value) {
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, value);
}
void ShaderProgram::setUniform(const std::string& varName, const glm::mat4& transform) {
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(transform));
}
void ShaderProgram::setUniform(const std::string& varName, const glm::vec3& color) {
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(color));
}
void ShaderProgram::setUniform(const std::string& structName, const Material& material) {
    int diffuseCount{0};
    int specularCount{0};
    int emissionCount{0};

    for (size_t textIndex = 0; textIndex < material.textures.size(); ++textIndex) {
        auto& texture = material.textures[textIndex];

        std::string uniformName = structName;
        switch (texture.type) {
            case TextureType::Diffuse:
                uniformName += ".diffuse";
                uniformName += "[" + std::to_string(diffuseCount++) + "]";
                setUniform(uniformName, texture.id);
                break;
            case TextureType::Specular:
                uniformName += ".specular";
                uniformName += "[" + std::to_string(specularCount++) + "]";
                setUniform(uniformName, texture.id);
                break;
            case TextureType::Emission:
                uniformName += ".emission";
                uniformName += "[" + std::to_string(emissionCount++) + "]";
                setUniform(uniformName, texture.id);
                break;
            default:
                break;
        }
    }

    setUniform(structName + ".diffuseTexturesNumber", diffuseCount);
    setUniform(structName + ".specularTexturesNumber", specularCount);
    setUniform(structName + ".emissionTexturesNumber", emissionCount);
    setUniform(structName + ".shininess", material.shininess);
    setUniform(structName + ".color", material.color);
}

void ShaderProgram::setUniform(const std::string& structName, const GlobalLight& light) {
    auto varName = structName + ".ambientIntence";
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.ambientIntence);

    varName = structName + ".diffuseIntence";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.diffuseIntence);

    varName = structName + ".specularIntence";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.specularIntence);

    varName = structName + ".color";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(light.color));

    varName = structName + ".position";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(light.position));
}

void ShaderProgram::setUniform(const std::string& structName, const PointLight& light) {
    auto varName = structName + ".ambientIntence";
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.ambientIntence);

    varName = structName + ".diffuseIntence";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.diffuseIntence);

    varName = structName + ".specularIntence";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.specularIntence);

    varName = structName + ".color";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(light.color));

    varName = structName + ".position";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(light.position));

    varName = structName + ".constant";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.constant);
    varName = structName + ".linear";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.linear);
    varName = structName + ".quadratic";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.quadratic);
}
void ShaderProgram::setUniform(const std::string& structName, const SpotLight& light) {
    auto varName = structName + ".ambientIntence";
    auto location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.ambientIntence);

    varName = structName + ".diffuseIntence";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.diffuseIntence);

    varName = structName + ".specularIntence";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.specularIntence);

    varName = structName + ".color";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(light.color));

    varName = structName + ".position";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(light.position));

    varName = structName + ".direction";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform3fv(location, 1, glm::value_ptr(light.direction));

    varName = structName + ".constant";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.constant);
    varName = structName + ".linear";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.linear);
    varName = structName + ".quadratic";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.quadratic);

    varName = structName + ".cutOff";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.cutOff);

    varName = structName + ".outerCutOff";
    location = glGetUniformLocation(programId_, varName.c_str());
    glUniform1f(location, light.outerCutOff);
}
