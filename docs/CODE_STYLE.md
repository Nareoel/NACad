# NACad code style

Two tools enforce these rules, and this document covers what they can't:

- **`.clang-format`** handles layout: Google base, 4-space indent, 110 columns, `SortIncludes: false`.
  Format every file you touch.
- **`.clang-tidy`** checks naming (`readability-identifier-naming`). CLion shows its warnings in the
  editor. From the terminal, run `clang-tidy -p <build-dir> <files>`. The build dir needs a
  `compile_commands.json`, so configure it with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

Rules marked *(manual)* can't be checked by clang-tidy, so they're checked in review.

## Naming

| Element | Rule | Example |
|---|---|---|
| Files *(manual)* | PascalCase, named after the main class, `.h` / `.cpp` | `ShaderProgram.h`, `Camera.cpp` |
| Classes, structs, enums | PascalCase | `Camera`, `PointLight`, `TextureType` |
| Enum values | PascalCase, always `enum class` | `TextureType::Diffuse`, `Camera::MovementType::Forward` |
| Type aliases | `using`, PascalCase | `using TextureID = unsigned int;` |
| Functions and methods | lowerCamelCase | `setModelTr()`, `processKeyboard()` |
| Private / protected methods | lowerCamelCase + trailing `_` | `init_()`, `processNode_()` |
| Private / protected data members | lowerCamelCase + trailing `_` | `position_`, `modelTr_`, `vao_` |
| Public struct fields | lowerCamelCase, no suffix | `shininess`, `ambientIntence` |
| Variables and parameters | lowerCamelCase | `deltaTime`, `filePath` |
| Constants (global, namespace-scope and `constexpr`) | `c` + PascalCase | `cPointLightsNumber`, `cDefaultColor`, `cSensitivity` |
| File-local helper functions *(manual)* | in an anonymous namespace, `s` + PascalCase | `sLoadImage()`, `sSetupImGui()` |
| Namespaces | PascalCase (exception: the alias `namespace fs = std::filesystem;`) | `Utils` |
| Macros | UPPER_CASE (avoid macros where possible) | `GLFW_INCLUDE_NONE` |

Abbreviations *(manual)*: `Tr` means transform (`localTr_`, `projectionTr`). Spell other words in full.

## Files and headers

- Every header starts with `#pragma once`.
- Include order in a `.cpp`: its own header first, then other project headers, then third-party and
  standard headers. In a header: standard, then third-party, then project headers. Includes aren't
  sorted automatically, so keep this order by hand.
- File-local helpers and constants go in an anonymous namespace (`namespace { ... }  // namespace`),
  never `static`.

## C++ usage

- Initialise members with default member initializers in braces: `double yaw_{-90.0};`.
- Functions that can fail return `std::optional` (see `ShaderProgram::createShaderProgram`).
- Take parameters by `const&` when they aren't cheap to copy. Use `auto` when the type is obvious.
- Prefer range-based `for` and `std::ranges` / `std::views`.

## Comments

- **Comment sparingly.** Add a comment only when the code can't be understood without it: a non-obvious
  why, a math formula or paper reference, a workaround, an invariant the code relies on.
- Don't restate what the code says (`// increment i`, `// returns the version`), don't narrate steps,
  and don't leave commented-out code. Prefer clearer names over a comment.
- Comments use `//` and start in lowercase.
