#include "ShaderProgram.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <string>
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
void ShaderProgram::clearMaterial(const std::string& structName) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    for (auto& textureTypeName : {"diffuse", "specular", "emission"}) {
        auto textureLayersUniformName = structName + "." + textureTypeName + "LayersIndices";
        auto countLayersUniformName = structName + "." + textureTypeName + "LayersCount";
        setUniform(textureLayersUniformName, 0);
        setUniform(countLayersUniformName, 0);
    }
}
void ShaderProgram::setUniform(const std::string& structName, const Material& material) {
    if (material.textureData) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, material.textureData->id);
        setUniform("textureArray", 0);
        auto fromTextureTypeToString = [](const TextureType& type) {
            switch (type) {
                case TextureType::Diffuse:
                    return "diffuse";
                case TextureType::Specular:
                    return "specular";
                case TextureType::Emission:
                    return "emission";
                default:
                    return "";
            }
        };
        for (const auto& textureType : {TextureType::Diffuse, TextureType::Specular, TextureType::Emission}) {
            auto countLayersUniformName =
                structName + "." + fromTextureTypeToString(textureType) + "LayersCount";
            auto textureLayersIt = material.textureData->textures.find(textureType);
            if (textureLayersIt == material.textureData->textures.end()) {
                setUniform(countLayersUniformName, 0);
                continue;
            }
            setUniform(countLayersUniformName, static_cast<int>(textureLayersIt->second.size()));

            auto textureLayersUniformName =
                structName + "." + fromTextureTypeToString(textureType) + "LayersIndices";
            for (size_t i = 0; i < textureLayersIt->second.size(); ++i) {
                auto layerUniformName = textureLayersUniformName + "[" + std::to_string(i) + "]";
                setUniform(layerUniformName, textureLayersIt->second[i]);
            }
        }
    }
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
