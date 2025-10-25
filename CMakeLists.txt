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
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# --- Optional: separate output folders ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)

include(FetchContent)

# -----------------------------------------------------------
# Dependencies
# -----------------------------------------------------------

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

# --- GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# --- ASSIMP ---
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.1
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# --- stb (header-only) --- stb_image.cpp is in src folder
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
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

find_package(OpenGL REQUIRED)

target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})

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
