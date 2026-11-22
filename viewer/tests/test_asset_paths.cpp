#include "core/AssetPaths.h"

#include <doctest/doctest.h>
#include <filesystem>
#include <set>

namespace {
using std::filesystem::path;

auto sAssetsIn(std::set<path> dirs) {
    return [dirs = std::move(dirs)](const path& dir) { return dirs.contains(dir); };
}
}  // namespace

TEST_CASE("asset root is the build root above bin/") {
    CHECK(Viewer::findAssetRoot("/x/build/bin/NACad", "/home", sAssetsIn({"/x/build"})) == path("/x/build"));
}

TEST_CASE("asset root is the build root even when the CWD also has assets") {
    CHECK(Viewer::findAssetRoot("/x/build/bin/NACad", "/home", sAssetsIn({"/x/build", "/home"})) ==
          path("/x/build"));
}

TEST_CASE("asset root is the executable dir when the assets sit next to the executable") {
    CHECK(Viewer::findAssetRoot("/x/app/NACad", "/home", sAssetsIn({"/x/app"})) == path("/x/app"));
}

TEST_CASE("asset root is the CWD when only the CWD has assets") {
    CHECK(Viewer::findAssetRoot("/x/build/bin/NACad", "/work", sAssetsIn({"/work"})) == path("/work"));
}

TEST_CASE("asset root falls back to the CWD when no candidate has assets") {
    CHECK(Viewer::findAssetRoot("/x/build/bin/NACad", "/work", sAssetsIn({})) == path("/work"));
}

TEST_CASE("asset root handles an unknown executable path") {
    int calls = 0;
    const auto hasAssets = [&calls](const path& dir) {
        ++calls;
        return dir == path("/work");
    };
    CHECK(Viewer::findAssetRoot(path{}, "/work", hasAssets) == path("/work"));
    CHECK(Viewer::findAssetRoot("NACad", "/work", hasAssets) == path("/work"));
    CHECK(calls == 2);  // empty candidates are skipped, only the CWD is checked
}

TEST_CASE("asset root is the CWD root when the executable path is unknown (web)") {
    CHECK(Viewer::findAssetRoot(path{}, "/", sAssetsIn({"/"})) == path("/"));
}
