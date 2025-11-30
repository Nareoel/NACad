cmake_minimum_required(VERSION 3.15)
project(NACad LANGUAGES C CXX)

cmake_policy(SET CMP0077 NEW) # respect BUILD_SHARED_LIBS

# -----------------------------------------------------------
# Compiler and build options
# -----------------------------------------------------------
set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(FetchContent)

# --- Optional: separate output folders ---
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/externals)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# -----------------------------------------------------------
# Dependencies
# -----------------------------------------------------------

# --- GLAD ---
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/_deps/glad-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/glad-build"
) 
FetchContent_MakeAvailable(glad)

# --- GLFW ---
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/_deps/glfw-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/glfw-build"
)
FetchContent_MakeAvailable(glfw)

# --- GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/_deps/glm-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/glm-build"
)
FetchContent_MakeAvailable(glm)

# --- ASSIMP ---
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.1
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/_deps/assimp-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/assimp-build"
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# --- stb (header-only) --- stb_image.cpp is in src folder
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/_deps/stb-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/stb-build"
)
FetchContent_MakeAvailable(stb)

# --- ImGui setup ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.0
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/_deps/imgui-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/imgui-build"
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

find_package(OpenGL REQUIRED)

target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})

# --- native file dialog extended ---
FetchContent_Declare(
    nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG v1.2.1
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/_deps/nfd-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/nfd-build"
)
FetchContent_MakeAvailable(nfd)

# -----------------------------------------------------------
# Project sources
# -----------------------------------------------------------
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/*.c
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

add_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${stb_SOURCE_DIR}
    ${nfd_SOURCE_DIR}/include
)

# -----------------------------------------------------------
# Link everything
# -----------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    glad
    glfw
    assimp
    glm
    imgui
    nfd
    ${OPENGL_LIBRARIES}
)

if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl pthread)
endif()

# -----------------------------------------------------------
# Copy shaders, samples, and shared libs
# -----------------------------------------------------------
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SAMPLES_DIR ${CMAKE_SOURCE_DIR}/samples)
set(DEST_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR} ${DEST_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SAMPLES_DIR} ${DEST_DIR}/samples
    COMMENT "Copying shaders and samples..."
)

# --- Copy shared libraries (DLL/.so) to build dir ---
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shared libraries..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:assimp>
    $<TARGET_FILE:glfw>
    ${DEST_DIR}
    VERBATIM
)

# -----------------------------------------------------------
# Message
# -----------------------------------------------------------
message(STATUS "All dependencies fetched, built as shared libs, and linked successfully!")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "ImGui source directory: ${imgui_SOURCE_DIR}")
