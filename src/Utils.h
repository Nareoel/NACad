#pragma once

#include <optional>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#include <stb_image.h>
#include <stb_image_resize2.h>

#include "ShaderProgram.h"

using TextureID = unsigned int;
namespace Utils {

using TextureInfo = std::pair<TextureID, std::unordered_map<std::filesystem::path, TextureLayerIndex>>;
TextureInfo createTextureFromImages(const std::unordered_set<std::filesystem::path>& uniqueImages);
}  // namespace Utils