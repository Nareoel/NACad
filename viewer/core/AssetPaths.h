#pragma once

#include <filesystem>
#include <functional>

namespace Viewer {

// returns the first of <exe dir>/.., <exe dir>, currentDir for which hasAssets is true,
// or currentDir if none matches
std::filesystem::path findAssetRoot(const std::filesystem::path& executable,
                                    const std::filesystem::path& currentDir,
                                    const std::function<bool(const std::filesystem::path&)>& hasAssets);

}  // namespace Viewer
