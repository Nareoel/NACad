#pragma once

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

enum class TextureType { Diffuse, Specular, Emission };
using TextureID = unsigned int;
using TextureLayerIndex = int;

struct TextureData {
    TextureID id;
    std::unordered_map<TextureType, std::vector<TextureLayerIndex>> textures;
};

struct Material {
    glm::vec3 color;
    std::optional<TextureData> textureData;
    float shininess = 32;
};

struct GlobalLight {
    glm::vec3 color = glm::vec3(1.0);
    // position is a synonim to -lightDirection
    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);

    float ambientIntence = 0.3;
    float diffuseIntence = 0.7;
    float specularIntence = 1.0;
};

struct PointLight {
    glm::vec3 color = glm::vec3(1.0);
    // position is a synonim to -lightDirection
    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);

    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;

    float ambientIntence = 0.3;
    float diffuseIntence = 0.7;
    float specularIntence = 1.0;
};

struct SpotLight {
    glm::vec3 color = glm::vec3(1.0);
    // position is a synonim to -lightDirection
    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 direction = glm::vec3(1.0, 0.0, 0.0);

    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));

    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;

    float ambientIntence = 0.;
    float diffuseIntence = 1.0;
    float specularIntence = 1.0;
};

class ShaderProgram {
   public:
    static std::optional<ShaderProgram> createShaderProgram(const std::filesystem::path& vShaderPath,
                                                            const std::filesystem::path& sShaderPath);
    void use();
    void setUniform(const std::string& varName, bool value);
    void setUniform(const std::string& varName, int value);
    void setUniform(const std::string& varName, unsigned int value);
    void setUniform(const std::string& varName, float value);
    void setUniform(const std::string& varName, const glm::mat4& transform);
    void setUniform(const std::string& varName, const glm::vec3& color);
    void setUniform(const std::string& structName, const Material& material);
    void clearMaterial(const std::string& structName);
    void setUniform(const std::string& structName, const GlobalLight& light);
    void setUniform(const std::string& structName, const SpotLight& light);
    void setUniform(const std::string& structName, const PointLight& light);

   private:
    ShaderProgram(unsigned int id);
    unsigned int programId_;
};
