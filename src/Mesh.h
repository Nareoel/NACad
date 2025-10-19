#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "ShaderProgram.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh {
   public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<int>& indices, const Material& material);
    void draw(ShaderProgram& shader) const;
    void setMaterial(const Material& material);
    void setLocalTr(const glm::mat4& tr);
    void resetLocalTr();
    void setModelTr(const glm::mat4& tr);
    void resetModelTr();

   private:
    std::vector<Vertex> vertices_;
    std::vector<int> indices_;
    Material material_;
    unsigned int VAO, VBO, EBO;
    glm::mat4 modelTr_{glm::mat4(1.0f)};
    glm::mat4 localTr_{glm::mat4(1.0f)};

    void init_();
};

std::shared_ptr<Mesh> createCubeMesh(const Material& material);