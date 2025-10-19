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

   private:
    std::vector<Mesh> meshes_;
    std::filesystem::path directory_;
    std::unordered_map<std::filesystem::path, Texture> loadedTextures_;

    void processNode_(aiNode* node, const aiScene* scene);
    Mesh convertFromAiMesh_(aiMesh* mesh, const aiScene* scene);
};