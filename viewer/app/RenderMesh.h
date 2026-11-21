#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "ShaderProgram.h"

namespace Viewer {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

// GPU-side mesh: owns its VAO/VBO/EBO, so it is move-only
class RenderMesh {
   public:
    RenderMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
               const Material& material);
    RenderMesh(const RenderMesh&) = delete;
    RenderMesh& operator=(const RenderMesh&) = delete;
    RenderMesh(RenderMesh&& other) noexcept;
    RenderMesh& operator=(RenderMesh&& other) noexcept;
    ~RenderMesh();

    void draw(ShaderProgram& shader) const;
    void setMaterial(const Material& material);
    void setLocalTr(const glm::mat4& tr);
    void resetLocalTr();
    void setModelTr(const glm::mat4& tr);
    void resetModelTr();

   private:
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    Material material_;
    unsigned int vao_{0};
    unsigned int vbo_{0};
    unsigned int ebo_{0};
    glm::mat4 modelTr_{glm::mat4(1.0f)};
    glm::mat4 localTr_{glm::mat4(1.0f)};

    void init_();
    void release_();
};

std::unique_ptr<RenderMesh> createCubeRenderMesh(const Material& material);

}  // namespace Viewer
