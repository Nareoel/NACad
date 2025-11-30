#include "Utils.h"
#include "ShaderProgram.h"

#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <ranges>

namespace {
struct ImageData {
    std::vector<unsigned char> data;
    int width;
    int height;
    int nrComponents;
};

std::optional<ImageData> sLoadImage(const std::filesystem::path& path) {
    int width;
    int height;
    int nrComponents;
    auto data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (!data) {
        stbi_image_free(data);
        return {};
    }

    ImageData imgData;
    imgData.data.assign(data, data + (width * height * nrComponents));
    stbi_image_free(data);
    imgData.width = width;
    imgData.height = height;
    imgData.nrComponents = nrComponents;
    return imgData;
}

}  // namespace

namespace Utils {

TextureInfo createTextureFromImages(const std::unordered_set<std::filesystem::path>& uniqueImagesPaths) {
    if (uniqueImagesPaths.empty()) {
        return {};
    }

    stbi_set_flip_vertically_on_load(true);

    // load images
    std::unordered_map<std::filesystem::path, ImageData> loadedImagesData;
    for (const auto& imagePath : uniqueImagesPaths) {
        auto maybeImgData = sLoadImage(imagePath);
        if (!maybeImgData.has_value()) {
            std::cout << "Failed to load image from path: " << imagePath << std::endl;
            continue;
        }
        std::cout << "Image loaded from path: " << imagePath << std::endl;
        loadedImagesData[imagePath] = std::move(*maybeImgData);
    }
    if (loadedImagesData.empty()) {
        return {};
    }

    // define base dimensions since for GL_TEXTURE_2D_ARRAY they should be the same
    int baseWidth = 0;
    int baseHeight = 0;
    int baseChannels = 0;
    for (const auto& imageData : loadedImagesData | std::views::values) {
        baseWidth = std::max(baseWidth, imageData.width);
        baseHeight = std::max(baseHeight, imageData.height);
        baseChannels = std::max(baseChannels, imageData.nrComponents);
    }

    TextureInfo textureInfo;
    // allocate GPU storage
    GLenum format;
    if (baseChannels == 1)
        format = GL_RED;
    else if (baseChannels == 3)
        format = GL_RGB;
    else if (baseChannels == 4)
        format = GL_RGBA;
    GLuint texArray;
    glGenTextures(1, &texArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, format, baseWidth, baseHeight,
                 static_cast<GLsizei>(loadedImagesData.size()), 0, format, GL_UNSIGNED_BYTE, nullptr);
    textureInfo.first = texArray;

    // resize and fill missing channels, upload to GPU
    int layerIdx{0};
    for (auto& [path, imageData] : loadedImagesData) {
        auto& width = imageData.width;
        auto& height = imageData.height;
        auto& channels = imageData.nrComponents;
        auto& data = imageData.data;

        // convert to baseChannels if needed
        if (channels != baseChannels) {
            std::vector<unsigned char> converted(width * height * baseChannels, 255);
            for (int pos = 0; pos < width * height; ++pos)
                for (int ch = 0; ch < std::min(channels, baseChannels); ++ch)
                    converted[pos * baseChannels + ch] = data[pos * channels + ch];
            data = std::move(converted);
        }

        // Resize to the largest dimensions
        std::vector<unsigned char> resized(baseWidth * baseHeight * baseChannels);
        stbir_resize_uint8_linear(data.data(), width, height, 0, resized.data(), baseWidth, baseHeight, 0,
                                  static_cast<stbir_pixel_layout>(baseChannels));

        // upload to GPU
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, static_cast<GLint>(layerIdx), baseWidth, baseHeight, 1,
                        format, GL_UNSIGNED_BYTE, resized.data());

        std::cout << "Image was uploaded to GPU: " << path << std::endl;

        textureInfo.second[path] = static_cast<TextureLayerIndex>(layerIdx);
        ++layerIdx;
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return textureInfo;
}

}  // namespace Utils
