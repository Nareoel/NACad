cmake_minimum_required(VERSION 3.18)
project(NACad LANGUAGES C CXX)

cmake_policy(SET CMP0077 NEW) # respect BUILD_SHARED_LIBS

# -----------------------------------------------------------
# Project options
# -----------------------------------------------------------
option(NACAD_BUILD_VIEWER "Build the NACad viewer (GLFW/OpenGL/ImGui/assimp)" ON)
option(NACAD_BUILD_TESTS "Build the unit tests (doctest + CTest)" ON)
option(NACAD_COVERAGE "Build the GL-free libraries and their tests with coverage instrumentation (GCC/Clang)" OFF)
option(NACAD_SANITIZE "Build the GL-free libraries and their tests with ASan + UBSan (GCC/Clang)" OFF)

# -----------------------------------------------------------
# Compiler and build options
# -----------------------------------------------------------
set(BUILD_SHARED_LIBS ON)
if(EMSCRIPTEN)
    # wasm has no shared libraries to copy next to the page
    set(BUILD_SHARED_LIBS OFF)
    # without it Emscripten aborts on any throw (doctest's REQUIRE throws)
    add_compile_options(-fexceptions)
    add_link_options(-fexceptions)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(FetchContent)

# Dependencies are downloaded and built inside each build dir (<build>/_deps), so build dirs never
# share FetchContent state (sharing it broke configures across generators and CMake versions).
# Some deps (e.g. glad) still declare cmake_minimum_required < 3.5, which CMake >= 4 rejects.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(NACAD_BUILD_TESTS)
    enable_testing()
endif()

# nacad_options (warnings) and nacad_dev_options (coverage, sanitizers)
include(${CMAKE_SOURCE_DIR}/cmake/NacadOptions.cmake)

# -----------------------------------------------------------
# Core dependencies (always needed)
# -----------------------------------------------------------

# --- Eigen (header-only) ---
# SOURCE_SUBDIR points at a nonexistent dir, so FetchContent only downloads Eigen and does not run
# its own (heavy) CMake. We provide the Eigen3::Eigen target ourselves.
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR do-not-add
)
FetchContent_MakeAvailable(eigen)

add_library(nacad_eigen INTERFACE)
target_include_directories(nacad_eigen SYSTEM INTERFACE ${eigen_SOURCE_DIR})
add_library(Eigen3::Eigen ALIAS nacad_eigen)

# --- doctest (header-only, tests only) ---
# Same download-only trick: doctest v2.4.11's CMakeLists requires CMake 3.0, which CMake >= 4 refuses.
if(NACAD_BUILD_TESTS)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR do-not-add
    )
    FetchContent_MakeAvailable(doctest)

    add_library(nacad_doctest INTERFACE)
    target_include_directories(nacad_doctest SYSTEM INTERFACE ${doctest_SOURCE_DIR})
    add_library(doctest::doctest ALIAS nacad_doctest)

    include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
endif()

# --- GLM (used by nacad_viewer_core, so tests-only builds need it too) ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# -----------------------------------------------------------
# Geometry library (GL-free) + its tests
# -----------------------------------------------------------
add_subdirectory(geom)

if(NACAD_BUILD_VIEWER)

# -----------------------------------------------------------
# Viewer dependencies
# -----------------------------------------------------------

if(EMSCRIPTEN)
    # --- GLFW (Emscripten's contrib.glfw3 port; WebGL2 replaces glad, no assimp or nfd yet) ---
    # the port flag must reach the compile step too: imgui_impl_glfw.cpp checks
    # EMSCRIPTEN_USE_PORT_CONTRIB_GLFW3, which the flag defines
    add_library(glfw INTERFACE)
    target_compile_options(glfw INTERFACE --use-port=contrib.glfw3)
    target_link_options(glfw INTERFACE --use-port=contrib.glfw3)
else()
    # --- GLAD ---
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v0.1.36
    )
    FetchContent_MakeAvailable(glad)

    # --- GLFW ---
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)

    # --- ASSIMP ---
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.3.1
    )
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(assimp)
endif()

# --- stb (header-only) --- stb_image.cpp is in viewer/app/
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    # commit before stb_image_resize2.h moved to deprecated/
    GIT_TAG 2b2e64bc4778df6a06fbea3a5dbe61f7a76ecf19
)
FetchContent_MakeAvailable(stb)

# --- ImGui setup ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.0
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

if(EMSCRIPTEN)
    # the OpenGL3 backend would pick GLSL ES 1.00 (WebGL1) on Emscripten otherwise
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)
    target_link_libraries(imgui PUBLIC glfw)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})

    # --- native file dialog extended ---
    FetchContent_Declare(
        nfd
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
        GIT_TAG v1.2.1
    )
    FetchContent_MakeAvailable(nfd)
endif()

if(BUILD_SHARED_LIBS)
    message(STATUS "All dependencies fetched, built as shared libs, and linked successfully!")
else()
    message(STATUS "All dependencies fetched, built as static libs, and linked successfully!")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "ImGui source directory: ${imgui_SOURCE_DIR}")

endif() # NACAD_BUILD_VIEWER

# -----------------------------------------------------------
# Viewer: GL-free core library + its tests, and (with NACAD_BUILD_VIEWER) the NACad executable
# -----------------------------------------------------------
add_subdirectory(viewer)
