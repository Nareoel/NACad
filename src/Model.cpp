#include "Model.h"
#include "Utils.h"

#include <iostream>
#include <ranges>
#include <unordered_set>

namespace {
const glm::vec3 defaultColor(1.0f, 0.925f, 0.5568f);
const std::unordered_map<aiTextureType, TextureType> cAiTextureTypeToOurTextureType{
    {aiTextureType_DIFFUSE, TextureType::Diffuse},
    {aiTextureType_SPECULAR, TextureType::Specular},
    {aiTextureType_EMISSIVE, TextureType::Emission}};

Model::ImagesInfo sLoadImagesInfoFromAssimpMaterial(aiMaterial* material, const std::filesystem::path& dir) {
    Model::ImagesInfo result;

    for (const auto& [assimpType, ourType] : cAiTextureTypeToOurTextureType) {
        for (size_t i = 0; i < material->GetTextureCount(assimpType); ++i) {
            aiString textureFileName;
            material->GetTexture(assimpType, i, &textureFileName);
            auto texturePath = dir / textureFileName.C_Str();
            result.emplace_back(ourType, std::move(texturePath));
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
    createTexturesAndSetMaterial_();
    std::cout << "Reading model file finished: " << filePath << std::endl;
}

void Model::draw(ShaderProgram& shader) const {
    for (const auto& mesh : meshesAndImagesInfo_ | std::views::keys) {
        mesh->draw(shader);
    }
}

void Model::processNode_(aiNode* node, const aiScene* scene) {
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        auto* mesh = scene->mMeshes[node->mMeshes[i]];
        loadFromAiMesh_(mesh, scene);
    }
    for (size_t i = 0; i < node->mNumChildren; ++i) {
        processNode_(node->mChildren[i], scene);
    }
}

void Model::loadFromAiMesh_(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<int> indices;

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

    auto ourMesh = std::make_unique<Mesh>(vertices, indices, Material{});

    if (mesh->mMaterialIndex > 0) {
        auto materials = scene->mMaterials[mesh->mMaterialIndex];
        auto images = sLoadImagesInfoFromAssimpMaterial(materials, directory_);
        meshesAndImagesInfo_.emplace(std::make_pair(std::move(ourMesh), images));
    }
}

void Model::createTexturesAndSetMaterial_() {
    std::unordered_set<std::filesystem::path> uniqueImages;
    for (const auto& images : meshesAndImagesInfo_ | std::views::values | std::views::join) {
        uniqueImages.insert(images.second);
    }
    const auto [textureId, textureLayersMap] = Utils::createTextureFromImages(uniqueImages);

    for (auto& [mesh, imagesInfo] : meshesAndImagesInfo_) {
        Material material;

        std::unordered_map<TextureType, std::vector<TextureLayerIndex>> textures;
        for (const auto& [textureType, imagePath] : imagesInfo) {
            if (auto textureLayersIt = textureLayersMap.find(imagePath);
                textureLayersIt != textureLayersMap.end()) {
                textures[textureType].emplace_back(textureLayersIt->second);
            }
        }
        if (textures.empty()) {
            material.color = defaultColor;
        } else {
            material.textureData = TextureData{.id = textureId, .textures = std::move(textures)};
        }
        mesh->setMaterial(material);
    }
}