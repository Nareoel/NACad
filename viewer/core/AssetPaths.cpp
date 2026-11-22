#include "core/AssetPaths.h"

#include <array>

namespace Viewer {

std::filesystem::path findAssetRoot(const std::filesystem::path& executable,
                                    const std::filesystem::path& currentDir,
                                    const std::function<bool(const std::filesystem::path&)>& hasAssets) {
    const auto executableDir = executable.parent_path();
    const std::array<std::filesystem::path, 3> candidates{executableDir.parent_path(), executableDir,
                                                          currentDir};
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && hasAssets(candidate)) {
            return candidate;
        }
    }
    return currentDir;
}

}  // namespace Viewer
