#include "RenderMesh.h"

#include <glad/glad.h>
#include <cstddef>
#include <utility>

namespace Viewer {

RenderMesh::RenderMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                       const Material& material)
    : vertices_{vertices}, indices_{indices}, material_{material} {
    init_();
}

RenderMesh::RenderMesh(RenderMesh&& other) noexcept
    : vertices_{std::move(other.vertices_)},
      indices_{std::move(other.indices_)},
      material_{std::move(other.material_)},
      vao_{std::exchange(other.vao_, 0)},
      vbo_{std::exchange(other.vbo_, 0)},
      ebo_{std::exchange(other.ebo_, 0)},
      modelTr_{other.modelTr_},
      localTr_{other.localTr_} {}

RenderMesh& RenderMesh::operator=(RenderMesh&& other) noexcept {
    if (this != &other) {
        release_();
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        material_ = std::move(other.material_);
        vao_ = std::exchange(other.vao_, 0);
        vbo_ = std::exchange(other.vbo_, 0);
        ebo_ = std::exchange(other.ebo_, 0);
        modelTr_ = other.modelTr_;
        localTr_ = other.localTr_;
    }
    return *this;
}

RenderMesh::~RenderMesh() { release_(); }

void RenderMesh::release_() {
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

void RenderMesh::init_() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                 vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_.size() * sizeof(unsigned int)),
                 indices_.data(), GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glBindVertexArray(0);
}

void RenderMesh::setLocalTr(const glm::mat4& tr) { localTr_ = tr; }
void RenderMesh::resetLocalTr() { localTr_ = glm::mat4(1.0f); }
void RenderMesh::setModelTr(const glm::mat4& tr) { modelTr_ = tr; }
void RenderMesh::resetModelTr() { modelTr_ = glm::mat4(1.0f); }

void RenderMesh::draw(ShaderProgram& shader) const {
    shader.setUniform("modelTr", modelTr_);
    shader.setUniform("localTr", localTr_);
    shader.setUniform("material", material_);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    shader.clearMaterial("material");
}
void RenderMesh::setMaterial(const Material& material) { material_ = material; }

std::unique_ptr<RenderMesh> createCubeRenderMesh(const Material& material) {
    std::vector<Vertex> vertices = {
        // position          // normal            // texture coords
        Vertex{glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},     // 0 -- 0
        Vertex{glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)},    // 1
        Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},   // 2
        Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},    // 3
        Vertex{glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)},    // 0 -- 1
        Vertex{glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},   // 1
        Vertex{glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},    // 2
        Vertex{glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)},     // 3
        Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f)},  // 0 -- 2
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f)},  // 1
        Vertex{glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f)},   // 2
        Vertex{glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f)},    // 3
        Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)},   // 0 -- 3
        Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},  // 1
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},  // 2
        Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)},   // 3
        Vertex{glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},    // 0 -- 4
        Vertex{glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},     // 1
        Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},    // 2
        Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)},   // 3
        Vertex{glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},   // 0 -- 5
        Vertex{glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},  // 1
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},  // 2
        Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f)}    // 3
    };

    std::vector<unsigned int> indices = {
        0,  1,  3,  1,  2,  3,   // 0 side
        4,  5,  7,  5,  6,  7,   // 1 side
        8,  9,  11, 9,  10, 11,  // 2 side
        12, 13, 15, 13, 14, 15,  // 3 side
        16, 17, 19, 17, 18, 19,  // 4 side
        20, 21, 23, 21, 22, 23,  // 5 side
    };

    return std::make_unique<RenderMesh>(vertices, indices, material);
}

}  // namespace Viewer