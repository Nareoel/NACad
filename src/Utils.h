#pragma once

#include <optional>
#include <stb_image.h>
#include <filesystem>

using TextureID = unsigned int;
namespace Utils {
std::optional<TextureID> loadTexture(const std::filesystem::path& imagePath);
}  // namespace Utils