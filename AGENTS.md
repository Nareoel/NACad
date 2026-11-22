# AGENTS.md

Guide for AI agents working in **NACad**, a C++20 / OpenGL 3.3 core 3D model viewer (planned editor),
built on the learnopengl.com approach. It also builds for the browser (Emscripten, WebGL2).

## Rules

- **Workflow** (commit/PR message format, secrets, gitleaks hook): [CONTRIBUTING.md](CONTRIBUTING.md).
- **Code style** (naming, files, C++ usage): [docs/CODE_STYLE.md](docs/CODE_STYLE.md). Naming is checked by
  `.clang-tidy`, layout by `.clang-format`.

## Build & run

```console
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/Debug
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release && cmake --build build/Release
```

- All dependencies come in through `FetchContent` and are downloaded and built inside each build dir
  (`<build>/_deps/`). Build dirs share nothing, so CLion and the terminal (any generator, any CMake
  version) can't break each other. The first configure of every build dir needs network access and is
  slow (assimp). `CMakeLists.txt` sets `CMAKE_POLICY_VERSION_MINIMUM 3.5`, so CMake ≥ 4 accepts glad's
  old `cmake_minimum_required` without extra flags.
- The executable goes to `<build>/bin/NACad`. A POST_BUILD step copies `shaders/` and `samples/` into
  `<build>/` and copies the assimp/glfw shared libs there too.
- **Assets are found next to the executable**, not via the CWD: at startup `App` takes the first of
  `<exe dir>/..` (the build root), `<exe dir>` and the CWD that contains `shaders/shader.vs`
  (`Viewer::findAssetRoot`), prints it as `Asset root: ...` and loads `shaders/`, `samples/` and
  `imgui.ini` from there. So `./build/Debug/bin/NACad` works from any folder, and so does CLion's
  default run configuration.
- At startup the console prints `OpenGL renderer: ...` and `GLFW platform: ...`; the ImGui window shows
  the FPS. On Linux GLFW uses Wayland when available; `NACAD_GLFW_PLATFORM=x11 ./bin/NACad` forces X11
  (XWayland). On hybrid-GPU laptops the app starts on the integrated GPU; `switcherooctl launch
  ./bin/NACad` runs it on the discrete one.
- `samples/` is **gitignored and empty in the repo**. At runtime `App::initScene_()` expects
  `samples/container.png`, `samples/containerMetalBorder.png`, `samples/matrix.jpg`,
  `samples/backpack/backpack.obj` (the learnopengl backpack) and
  `samples/spider/Only_Spider_with_Animations_Export.obj`. If they are missing, loading fails with
  console messages and nothing crashes.
- `cmake-build-debug/` is a CLion build dir and has `compile_commands.json`.
- Code style: `.clang-format` (Google base, 4-space indent, 110 column limit, `SortIncludes: false`, so
  keep the include order as it is).

### Web build (Emscripten)

```console
tools/setup-emsdk.sh                  # once: pinned emsdk 4.0.23 into externals/emsdk (gitignored)
source externals/emsdk/emsdk_env.sh   # in every new shell
emcmake cmake -S . -B build/Web -DCMAKE_BUILD_TYPE=Release && cmake --build build/Web
ctest --test-dir build/Web --output-on-failure        # the same doctest tests, run under Node
python3 -m http.server --directory build/Web/bin      # then open http://localhost:8000/NACad.html
```

- `tools/setup-emsdk.sh` is idempotent. If `$EMSDK` is already set it reuses that SDK (and warns when its
  version differs from the pin); otherwise it clones emsdk into `externals/emsdk`, checks out the pinned
  tag and runs `emsdk install` + `emsdk activate` (never `--permanent`, so your shell profile isn't
  touched). emsdk brings its own Node, and the port/library cache stays under `externals/emsdk`.
- Output: `build/Web/bin/NACad.{html,js,wasm,data}`. `.data` holds `shaders/` (preloaded at `/shaders`
  in the virtual file system) and the three container images from `samples/` (at `/samples/...`, only
  those that exist at configure time; re-run `emcmake cmake` after adding them, otherwise the
  containers render black); the page comes from `viewer/web/shell.html`. The page must be served over
  HTTP (`file://` can't fetch the `.wasm`/`.data`); `emrun build/Web/bin/NACad.html` works too.
- Tests: the Emscripten toolchain sets `CMAKE_CROSSCOMPILING_EMULATOR` to emsdk's `node`, which
  `doctest_discover_tests` and CTest use, so `ctest` runs the `.js` test binaries under Node. A
  tests-only web build (`-DNACAD_BUILD_VIEWER=OFF`) works as well.
- How the web build differs from the desktop one:
  - no models (assimp and nfd aren't built), so no backpack or spider; the containers are textured;
  - no glad: GL comes from `<GLES3/gl3.h>` through `viewer/app/GlApi.h`; GLFW is Emscripten's
    `contrib.glfw3` port; the GL context is WebGL2 (OpenGL ES 3.0);
  - the shader files keep `#version 330 core`; at load time `prepareShaderSource` swaps it for
    `#version 300 es` plus default precisions;
  - the app starts in interactive mode (free cursor), because the browser only grants pointer lock after
    a user gesture. I locks the pointer for mouse look; Esc (handled by the browser) releases it and
    goes back to interactive mode. The window can't be closed from inside, so Esc doesn't quit;
  - the console prints `OpenGL renderer`, `OpenGL version` and `Max fragment uniform vectors` (the
    shader's three `int[256]` layer arrays need ~800; WebGL2 only guarantees 224, desktop browsers give
    ~1024) instead of `GLFW platform`;
  - WebGL2 has no `glPolygonMode`, so a wireframe mode (roadmap day 11) needs an edge index buffer drawn
    with `GL_LINES` instead.
- **Browser support.** WebGL2 is required. Emscripten 4.0.23's defaults (`src/settings.js`) target
  Chrome ≥ 85 (`MIN_CHROME_VERSION`), Firefox ≥ 79 (`MIN_FIREFOX_VERSION`), Safari ≥ 15.0
  (`MIN_SAFARI_VERSION = 150000`) and Node ≥ 16.0 (`MIN_NODE_VERSION = 160000`). The build uses no
  threads, wasm SIMD or WebGPU, so the server needs no COOP/COEP headers.

### Options

| Option | Default | Effect |
|---|---|---|
| `NACAD_BUILD_VIEWER` | ON | Builds the `NACad` executable and its deps (glad, glfw, assimp, stb, imgui, nfd). OFF gives a fast tests-only build without assimp (`nacad_viewer_core` and glm are still built). |
| `NACAD_BUILD_TESTS` | ON | `enable_testing()`, fetches doctest, builds `nacad_geom_tests` and `nacad_viewer_tests`. |
| `NACAD_COVERAGE` | OFF | `--coverage -O0 -g` on `nacad_geom`, `nacad_viewer_core` + their tests (GCC/Clang) and a `coverage` target (needs `gcovr`; reports `geom/geom/` and `viewer/core/`). Native only: ignored with a warning in Emscripten builds. |
| `NACAD_SANITIZE` | OFF | ASan + UBSan on `nacad_geom`, `nacad_viewer_core` + their tests (GCC/Clang). Not applied to the `NACad` executable. Native only: ignored with a warning in Emscripten builds. |

### Tests

**All functionality must be covered by unit tests** (doctest + CTest); see the testing policy in
[CONTRIBUTING.md](CONTRIBUTING.md#tests). Each `TEST_CASE` is its own CTest test (`doctest_discover_tests`).

```console
# fast, tests only (no assimp)
cmake -S . -B build/Tests -DCMAKE_BUILD_TYPE=Debug -DNACAD_BUILD_VIEWER=OFF && cmake --build build/Tests
ctest --test-dir build/Tests --output-on-failure
# sanitizers
cmake -S . -B build/Asan -DCMAKE_BUILD_TYPE=Debug -DNACAD_BUILD_VIEWER=OFF -DNACAD_SANITIZE=ON && cmake --build build/Asan
ctest --test-dir build/Asan --output-on-failure
# coverage (HTML report in build/Coverage/coverage/index.html; the target runs ctest itself)
cmake -S . -B build/Coverage -DCMAKE_BUILD_TYPE=Debug -DNACAD_BUILD_VIEWER=OFF -DNACAD_COVERAGE=ON
cmake --build build/Coverage && cmake --build build/Coverage --target coverage
```

New test files go in `geom/tests/test_<module>.cpp` (geometry) or `viewer/tests/test_<module>.cpp`
(viewer core logic) and must be added to the explicit source list in `geom/CMakeLists.txt` or
`viewer/CMakeLists.txt` (as must new library and app sources; nothing uses GLOB).

## Dependencies (pinned in CMakeLists.txt)

Viewer only: glad v0.1.36, GLFW 3.4, assimp v5.3.1, stb (a pinned commit from before
`stb_image_resize2.h` moved to `deprecated/`), Dear ImGui v1.92.0 (built as a local static lib with
the glfw and opengl3 backends), nativefiledialog-extended v1.2.1 (linked but not used yet). The web
build fetches only stb and Dear ImGui (built with `IMGUI_IMPL_OPENGL_ES3`); GLFW is Emscripten's
`contrib.glfw3` port (an INTERFACE target `glfw` carrying `--use-port=contrib.glfw3` on compile and
link, so `imgui_impl_glfw.cpp` sees `EMSCRIPTEN_USE_PORT_CONTRIB_GLFW3`), and glad, assimp and nfd
aren't used. Emscripten builds everything static (`BUILD_SHARED_LIBS OFF`) with `-fexceptions`.
Always fetched: GLM 1.0.1 (needed by `nacad_viewer_core`), Eigen 3.4.0 and (with tests) doctest
v2.4.11. Eigen and doctest are download-only (`SOURCE_SUBDIR do-not-add`, so their own CMake never
runs) and are exposed through our own INTERFACE targets `Eigen3::Eigen` and `doctest::doctest` with
SYSTEM includes.

CMake helpers live in `cmake/NacadOptions.cmake`: `nacad_options` (warnings `-Wall -Wextra -Wpedantic`
/ `/W4`, linked by every NACad target), `nacad_dev_options` (coverage + sanitizers, linked only by
the GL-free libraries `nacad_geom`, `nacad_viewer_core` and their tests; native only) and
`nacad_test_options` (linked by the test executables; empty natively, and on the web
`DOCTEST_CONFIG_COLORS_NONE` plus `-sEXIT_RUNTIME=1 -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=node` so they
run under Node and report their exit code).

## `geom/`: the `nacad_geom` library

A GL-free static library (pure C++20 + Eigen; no OpenGL, GLFW, ImGui or assimp), CMake alias
`nacad::geom`. The viewer (`NACad`) links it. Each header sits next to its source in `geom/geom/`
(`Version.h` + `Version.cpp`) and is included with the prefix, `#include "geom/Version.h"`, which keeps
it apart from the viewer's own headers (e.g. `core/Camera.h`). Tests are in `geom/tests/`
(`test_main.cpp` only holds the doctest main). Namespace `Geom`. Currently only `Version.h`
(`Geom::version()` returns `"0.1.0"`). The geometry `Geom::Mesh` (half-edge) and friends will live here.


## Source map (`viewer/`, ~1.5k LOC)

Namespace `Viewer` (texture helpers in `Viewer::Utils`); `main()` is global. Sources are listed
explicitly in `viewer/CMakeLists.txt`, so add every new `.cpp` there.

### `viewer/core/`: the `nacad_viewer_core` library

A GL-free static library (C++20 + glm only), CMake alias `nacad::viewer_core`, included with the
prefix (`#include "core/Camera.h"`). Viewer logic that can be unit-tested goes here; its tests are in
`viewer/tests/` (`nacad_viewer_tests`).

| File | Role |
|---|---|
| `Camera.h/.cpp` | FPS-style `Camera` (yaw/pitch, pitch clamp ±89, FOV clamp 1–45, speed 2.5, mouse sensitivity 0.1). `resetView()` swaps with a default-constructed camera. Getters for position, front, up, yaw, pitch, FOV. |
| `Projection.h/.cpp` | `aspectRatio(width, height)` (returns `cDefaultAspectRatio` = 4/3 for an empty framebuffer, e.g. a minimised window) and `perspective(fovDeg, aspect)` (near 0.1, far 100). |
| `MouseLook.h/.cpp` | Turns cursor positions into offsets; `delta(x, y)` returns no delta on the first event, so the camera does not jump. `reset()` makes the next event a first event again; `App` calls it when interactive mode is toggled. |
| `AssetPaths.h/.cpp` | `findAssetRoot(exe, cwd, hasAssets)`: picks the asset folder from `<exe dir>/..`, `<exe dir>`, the CWD (fallback). The existence check is passed in, so tests don't touch the disk. On the web the executable path is empty and the CWD is `/`, so the root is `/`. |
| `ShaderSource.h/.cpp` | `ShaderDialect` (`Glsl330Core`, `GlslEs300`), `shaderHeader(dialect)` and `prepareShaderSource(src, dialect)`: replaces the first non-blank, non-`//` line if it is a `#version` directive with the dialect's header (or prepends the header, followed by `#line 1`, when there is none). Leading blank/comment lines stay above it; when the header is longer than one line, `#line N` restores the file's line numbers. A desktop file that starts with `#version 330 core` comes out byte-identical. |

### `viewer/app/`: the `NACad` executable (GL, GLFW, ImGui, assimp on the desktop)

Code under `__EMSCRIPTEN__` is the web build; everything else is unchanged on the desktop.

| File | Role |
|---|---|
| `main.cpp` | Entry point: `Viewer::App app; if (!app.init()) return 1; app.run();`. On the web `app` is `static`, because the browser keeps calling the main loop after `main()` has gone. |
| `GlApi.h` | The only place that includes the GL headers: `<GLES3/gl3.h>` on the web, `<glad/glad.h>` otherwise. App code includes `"GlApi.h"`, never glad directly. |
| `App.h/.cpp` | Owns all viewer state: window, `Camera`, `MouseLook`, timing, framebuffer size, light toggles and lights, materials, shader, cube `RenderMesh`, the backpack and spider `Model`s (the spider is ~200 units across, so it is scaled by 0.02), `interactiveMode_`. `init()` sets up GLFW, GLAD, the scene (`initScene_()`) and ImGui; `run()` loops over `frame_()` (input, `drawScene_()`, `drawImGui_()`). GLFW callbacks are free functions that get the `App*` from `glfwGetWindowUserPointer` and forward to `onFramebufferSize/onKey/onMouseMove/onScroll`. `setInteractiveMode(bool)` sets the mode, resets `MouseLook` and applies the cursor mode (the I key and, on the web, the pointer-lock change/error callbacks use it). The projection aspect comes from the framebuffer size. `shutdown()` (idempotent, also run by the destructor) frees the GL objects while the context is alive, then shuts down ImGui and GLFW. On the web: `run()` hands `frame_()` to `emscripten_set_main_loop_arg`, the context is ES 3.0 (WebGL2), ImGui uses `#version 300 es` and `ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas")` (the canvas follows the browser window size), the models are compiled out, and the app starts in interactive mode. The container material gets its `TextureData` only when the texture load returned a non-zero id. |
| `ShaderProgram.h/.cpp` | Defines the **core data types** (`TextureType`, `TextureData`, `Material`, `GlobalLight`, `PointLight`, `SpotLight`) and the move-only `ShaderProgram` class (destructor calls `glDeleteProgram`): compile/link through the `createShaderProgram(vs, fs)` factory (returns `std::optional`; each source goes through `prepareShaderSource` with `Glsl330Core`, or `GlslEs300` on the web) plus `setUniform` overloads for scalars, `vec3`, `mat4` and each struct. |
| `RenderMesh.h/.cpp` | `Vertex` (pos, normal, uv) and the move-only `RenderMesh` (owns VAO/VBO/EBO and frees them in its destructor, material, `modelTr_` + `localTr_`, `unsigned int` indices). `draw()` sets the transforms and material, draws, then calls `clearMaterial`. Also `createCubeRenderMesh()` (24 verts, 36 indices). The name `Mesh` is reserved for `Geom::Mesh`. |
| `Model.h/.cpp` | Loads a file with assimp (`Triangulate | FlipUVs`), walks the nodes recursively, turns each `aiMesh` into a `RenderMesh`, then packs every unique texture into **one** texture array (owned by the `Model`, which is move-only) and assigns per-mesh `Material`s. Every mesh is kept; one with no textures (or an out-of-range material index) gets the default color. `setModelTr(mat4)` applies a transform to all its meshes. |
| `Utils.h/.cpp` | `Utils::createTextureFromImages(set<path>)` loads the images with stb, normalises them to the max width/height/channels (resizing with `stbir_resize_uint8_linear`), uploads them as layers of a `GL_TEXTURE_2D_ARRAY` (sized internal format `GL_R8`/`GL_RG8`/`GL_RGB8`/`GL_RGBA8`, which WebGL2 requires, with the matching unsized external format) and returns `{textureId, path -> layerIndex}`. The caller owns the texture. |
| `../web/shell.html` | The page template for the web build (`--shell-file`): a full-window `<canvas id="canvas">`, `Module.print`/`printErr` routed to the browser console. |
| `stb_image.cpp` | The single translation unit that holds `STB_IMAGE_IMPLEMENTATION` and `STB_IMAGE_RESIZE2_IMPLEMENTATION`. Don't define these anywhere else. |

## Rendering architecture

- **Single shader** for everything: `shaders/shader.vs` + `shaders/shader.fs`. The files are written as
  `#version 330 core`; `ShaderProgram` runs them through `prepareShaderSource`, which on the web swaps
  that line for `#version 300 es` + `precision highp float/int/sampler2DArray` (and a `#line` so error
  line numbers still match the file). Keep the GLSL valid in both dialects (explicit `float(...)`
  conversions, no desktop-only built-ins). `shaderLightSource.fs`
  is currently unused, and the light cubes are drawn with the main shader using a solid `material.color`.
- Transform chain: `projectionTr * viewTr * modelTr * localTr * pos`.
- Lighting is Phong with one `globalLight` (directional, where `position` means −direction), `pointlights[4]`
  and one `spotLight` (a flashlight attached to the camera).
- **Texture model**: a material does not hold separate textures. It holds one texture-array id plus,
  for each `TextureType`, a list of layer indices. The fragment shader averages all the layers of each
  type. Emission scrolls with the `time` uniform and is masked where specular is present.
  `material.color` is added on top, so it should be black (0) for textured meshes.
- Uniform names are built as strings in C++ and **must match the GLSL exactly**:
  - `material.{diffuse,specular,emission}LayersIndices[i]` / `...LayersCount`, `material.shininess`, `material.color`
  - light fields use the misspelling `ambientIntence` / `diffuseIntence` / `specularIntence` on both
    sides. Rename both or neither.
  - the array is called `pointlights` (lowercase L).
  - `cPointLightsNumber` in `App.cpp` must equal `NR_POINT_LIGHTS` in `shader.fs`.
  - GLSL arrays hold at most 256 layer indices per type.
- Projection: `Viewer::perspective(camera FOV, Viewer::aspectRatio(framebuffer size))`, so resizing the
  window keeps proportions (the 1600×1200 start window gives 4:3).

## Controls

WASD move, Space/C up/down, mouse look, scroll zoom, F reset camera, G/P/Q toggle the
global/point/spot light, I toggle interactive mode (frees the cursor for ImGui), Esc quit. On the web
the app starts in interactive mode, I locks the pointer, and Esc releases it (back to interactive mode)
instead of quitting.

## Known quirks and pitfalls (verify before you rely on them)

- Meshes without textures render as flat unlit color, because `shader.fs` multiplies the lighting by
  the texture colors.
- The viewer compiles with `-Wall -Wextra -Wpedantic` (via `nacad_options`) and has no warnings; keep it
  that way (there is no `-Werror`).
- On GNOME Wayland, interactive window resizing drops to ~10 FPS and looks jerky (GLFW 3.4's Wayland
  resize handshake, not our rendering: the same build stays at 60 FPS under X11). Wayland is kept as the
  default on purpose; revisit after a GLFW update.
- The ImGui window holds placeholder text plus the FPS readout.
