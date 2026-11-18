# AGENTS.md

Guide for AI agents working in **NACad**, a C++20 / OpenGL 3.3 core 3D model viewer (planned editor),
built on the learnopengl.com approach.

## Rules

- **Workflow** (commit/PR message format, secrets, gitleaks hook): [CONTRIBUTING.md](CONTRIBUTING.md).
- **Code style** (naming, files, C++ usage): [docs/CODE_STYLE.md](docs/CODE_STYLE.md). Naming is checked by
  `.clang-tidy`, layout by `.clang-format`.

## Build & run

```console
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/Debug
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release && cmake --build build/Release
```

- All dependencies come in through `FetchContent` and are cloned into `externals/_deps/` (gitignored).
  The first configure needs network access and is slow (assimp).
- The executable goes to `<build>/bin/NACad`. A POST_BUILD step copies `shaders/` and `samples/` into
  `<build>/` and copies the assimp/glfw shared libs there too.
- **Asset paths are relative to the CWD** (`"shaders/shader.vs"`, `"samples/..."`), so run it from the
  build root: `cd build/Debug && ./bin/NACad`.
- `samples/` is **gitignored and empty in the repo**. At runtime `main.cpp` expects
  `samples/container.png`, `samples/containerMetalBorder.png`, `samples/matrix.jpg`,
  `samples/backpack/backpack.obj` (the learnopengl backpack) and
  `samples/spider/Only_Spider_with_Animations_Export.obj`. If they are missing, loading fails with
  console messages and nothing crashes.
- `cmake-build-debug/` is a CLion build dir and has `compile_commands.json`.
- There are no tests.
- Code style: `.clang-format` (Google base, 4-space indent, 110 column limit, `SortIncludes: false`, so
  keep the include order as it is).

## Dependencies (pinned in CMakeLists.txt)

glad v0.1.36, GLFW 3.4, GLM 1.0.1, assimp v5.3.1, stb (a pinned commit from before
`stb_image_resize2.h` moved to `deprecated/`), Dear ImGui v1.92.0 (built as a local static lib with
the glfw and opengl3 backends), nativefiledialog-extended v1.2.1 (linked but not used yet).

## Source map (`src/`, ~1.2k LOC, flat, picked up by `GLOB_RECURSE`)

New `.cpp` files are added to the build automatically, but you need to re-run the CMake configure.

| File | Role |
|---|---|
| `main.cpp` | Entry point. Sets up GLFW, GLAD and ImGui, builds the scene (lights, container material, cube mesh, backpack and spider models; the spider is ~200 units across, so it is scaled by 0.02), runs the render loop, handles input callbacks. Holds the global state (`camera`, light toggles, `interactiveMode`, timing). |
| `ShaderProgram.h/.cpp` | Defines the **core data types** (`TextureType`, `TextureData`, `Material`, `GlobalLight`, `PointLight`, `SpotLight`) and the `ShaderProgram` class: compile/link through the `createShaderProgram(vs, fs)` factory (returns `std::optional`) plus `setUniform` overloads for scalars, `vec3`, `mat4` and each struct. |
| `Mesh.h/.cpp` | `Vertex` (pos, normal, uv) and `Mesh` (VAO/VBO/EBO, material, `modelTr_` + `localTr_`). `draw()` sets the transforms and material, draws, then calls `clearMaterial`. Also `createCubeMesh()` (24 verts, 36 indices). |
| `Model.h/.cpp` | Loads a file with assimp (`Triangulate | FlipUVs`), walks the nodes recursively, turns each `aiMesh` into a `Mesh`, then packs every unique texture into **one** texture array and assigns per-mesh `Material`s. Every mesh is kept; one with no textures (or an out-of-range material index) gets the default color. `setModelTr(mat4)` applies a transform to all its meshes. |
| `Utils.h/.cpp` | `Utils::createTextureFromImages(set<path>)` loads the images with stb, normalises them to the max width/height/channels (resizing with `stbir_resize_uint8_linear`), uploads them as layers of a `GL_TEXTURE_2D_ARRAY` and returns `{textureId, path -> layerIndex}`. |
| `Camera.h/.cpp` | FPS-style `Camera` (yaw/pitch, FOV clamp 1–45, speed 2.5). `resetView()` swaps with a default-constructed camera. |
| `stb_image.cpp` | The single translation unit that holds `STB_IMAGE_IMPLEMENTATION` and `STB_IMAGE_RESIZE2_IMPLEMENTATION`. Don't define these anywhere else. |

## Rendering architecture

- **Single shader** for everything: `shaders/shader.vs` + `shaders/shader.fs`. `shaderLightSource.fs`
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
  - `cPointLightsNumber` in `main.cpp` must equal `NR_POINT_LIGHTS` in `shader.fs`.
  - GLSL arrays hold at most 256 layer indices per type.
- Projection aspect is hardcoded to `800/600` while the window is 1600×1200.

## Controls

WASD move, Space/C up/down, mouse look, scroll zoom, F reset camera, G/P/Q toggle the
global/point/spot light, I toggle interactive mode (frees the cursor for ImGui), Esc quit.

## Known quirks and pitfalls (verify before you rely on them)

- Meshes without textures render as flat unlit color, because `shader.fs` multiplies the lighting by
  the texture colors.
- `Utils::createTextureFromImages` uses the same `format` (GL_RED/RGB/RGBA) as both internal and
  external format. `format` is uninitialised if the image has 2 channels.
- `CMakeLists.txt`: `add_compile_options(${PROJECT_NAME} PRIVATE ...)` is called after `add_executable`
  with target-style arguments, so the warning flags do not actually apply to the target (the intended
  call is `target_compile_options`). The `include/` dir it adds does not exist.
- `Mesh` stores indices as `std::vector<int>` but uploads them as `GL_UNSIGNED_INT` (same size, OK).
- `Mesh` has no destructor or move semantics, so GL buffers are never freed and copying a `Mesh` aliases them.
- `main.cpp` has a `// ToDo remove global variables` note. The ImGui window is a placeholder.
