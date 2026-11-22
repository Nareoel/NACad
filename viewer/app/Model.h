#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "RenderMesh.h"

namespace Viewer {

// owns its meshes and the texture array they share, so it is move-only
class Model {
   public:
    explicit Model(const std::filesystem::path& filePath);
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;
    ~Model();

    void loadModel(const std::filesystem::path& filePath);
    void draw(ShaderProgram& shader) const;
    void setModelTr(const glm::mat4& tr);

    using ImagesInfo = std::vector<std::pair<TextureType, std::filesystem::path>>;

   private:
    std::filesystem::path directory_;
    std::unordered_map<std::unique_ptr<RenderMesh>, ImagesInfo> meshesAndImagesInfo_;
    TextureID textureId_{0};

    void processNode_(aiNode* node, const aiScene* scene);
    void loadFromAiMesh_(aiMesh* mesh, const aiScene* scene);
    void createTexturesAndSetMaterial_();
    void release_();
};

}  // namespace Viewer
