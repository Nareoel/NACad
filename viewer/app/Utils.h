#pragma once

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "ShaderProgram.h"

namespace Viewer::Utils {

using TextureInfo = std::pair<TextureID, std::unordered_map<std::filesystem::path, TextureLayerIndex>>;
TextureInfo createTextureFromImages(const std::unordered_set<std::filesystem::path>& uniqueImages);

}  // namespace Viewer::Utils
