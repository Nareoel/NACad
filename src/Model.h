#pragma once

#include "Mesh.h"
#include <filesystem>
#include <unordered_map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model {
   public:
    Model(const std::filesystem::path& file);
    void loadModel(const std::filesystem::path& file);
    void draw(ShaderProgram& shader) const;

    using ImagesInfo = std::vector<std::pair<TextureType, std::filesystem::path>>;

   private:
    std::filesystem::path directory_;
    std::unordered_map<std::unique_ptr<Mesh>, ImagesInfo> meshesAndImagesInfo_;

    void processNode_(aiNode* node, const aiScene* scene);
    void loadFromAiMesh_(aiMesh* mesh, const aiScene* scene);
    void createTexturesAndSetMaterial_();
};