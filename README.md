# NACad
3D Models viewer and editor

Inspired by https://learnopengl.com/

Debug build

```console
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/Debug
```

Release build

```console
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release && cmake --build build/Release
```

## License

NACad is released under the [MIT License](LICENSE). You may copy, modify and redistribute it, as long as
the copyright notice and the license text are kept.

## Third-party libraries

The dependencies are not part of this repository: CMake downloads them at configure time (`FetchContent`).

| Library | Version | License | Used for |
|---|---|---|---|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | zlib | window, input |
| [glad](https://github.com/Dav1dde/glad) | v0.1.36 | MIT | OpenGL loader (desktop) |
| [Dear ImGui](https://github.com/ocornut/imgui) | v1.92.0 | MIT | UI |
| [assimp](https://github.com/assimp/assimp) | v5.3.1 | BSD-3-Clause | model loading (desktop) |
| [stb](https://github.com/nothings/stb) | pinned commit | MIT or public domain | image loading and resizing |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | MIT | viewer math |
| [Eigen](https://gitlab.com/libeigen/eigen) | 3.4.0 | MPL-2.0 | geometry library math |
| [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) | v1.2.1 | zlib | file dialogs (desktop) |
| [doctest](https://github.com/doctest/doctest) | v2.4.11 | MIT | unit tests |
