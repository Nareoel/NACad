#include "Model.h"
#include "Utils.h"

#include <iostream>

namespace {
const glm::vec3 defaultColor(1.0f, 0.925f, 0.5568f);

std::vector<Texture> sLoadTextureFromAssimpMaterial(
    aiMaterial* material, const std::filesystem::path& dir,
    std::unordered_map<std::filesystem::path, Texture>& preloadedTextures) {
    std::vector<Texture> result;

    const std::vector<std::pair<aiTextureType, TextureType>> cNeededTextures{
        {aiTextureType_DIFFUSE, TextureType::Diffuse},
        {aiTextureType_SPECULAR, TextureType::Specular},
        {aiTextureType_EMISSIVE, TextureType::Emission}};

    for (const auto& [assimpType, ourType] : cNeededTextures) {
        for (size_t i = 0; i < material->GetTextureCount(assimpType); ++i) {
            aiString textureFileName;
            material->GetTexture(assimpType, i, &textureFileName);
            auto texturePath = dir / textureFileName.C_Str();
            if (preloadedTextures.contains(texturePath)) {
                result.emplace_back(preloadedTextures[texturePath]);
            } else {
                auto maybeTextureId = Utils::loadTexture(texturePath);
                if (!maybeTextureId) {
                    continue;
                }
                Texture texture{.id = *maybeTextureId, .type = ourType};
                result.emplace_back(texture);
                preloadedTextures[texturePath] = texture;
            }
        }
    }

    return result;
};

}  // namespace

Model::Model(const std::filesystem::path& filePath) { loadModel(filePath); }

void Model::loadModel(const std::filesystem::path& filePath) {
    std::cout << "Reading model file: " << filePath << std::endl;
    Assimp::Importer importer;
    const auto* scene = importer.ReadFile(filePath.string(), aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "Error: read model file failed: " << filePath << std::endl;
        return;
    }
    directory_ = filePath.parent_path();
    processNode_(scene->mRootNode, scene);
    std::cout << "Reading model file finished: " << filePath << std::endl;
}

void Model::draw(ShaderProgram& shader) const {
    for (const auto& mesh : meshes_) {
        mesh.draw(shader);
    }
}

void Model::processNode_(aiNode* node, const aiScene* scene) {
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        auto* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes_.push_back(convertFromAiMesh_(mesh, scene));
    }
    for (size_t i = 0; i < node->mNumChildren; ++i) {
        processNode_(node->mChildren[i], scene);
    }
}

Mesh Model::convertFromAiMesh_(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<int> indices;
    Material material;

    vertices.reserve(mesh->mNumVertices);
    for (size_t i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) {
            vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }

        vertices.emplace_back(vertex);
    }

    indices.reserve(3 * mesh->mNumFaces);
    for (size_t i = 0; i < mesh->mNumFaces; ++i) {
        auto face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; ++j) {
            indices.emplace_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex > 0) {
        material.color = glm::vec3(0, 0, 0);
        auto materials = scene->mMaterials[mesh->mMaterialIndex];
        material.textures = sLoadTextureFromAssimpMaterial(materials, directory_, loadedTextures_);
    }
    if (material.textures.empty()) {
        material.color = defaultColor;
    }

    return Mesh(vertices, indices, material);
}