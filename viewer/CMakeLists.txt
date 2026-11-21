# -----------------------------------------------------------
# nacad_viewer_core: GL-free viewer logic (camera, projection, mouse look, asset paths), glm only
# -----------------------------------------------------------
add_library(nacad_viewer_core STATIC
    core/AssetPaths.cpp
    core/Camera.cpp
    core/MouseLook.cpp
    core/Projection.cpp
)
add_library(nacad::viewer_core ALIAS nacad_viewer_core)

# headers are included with the prefix, e.g. "core/Camera.h"
target_include_directories(nacad_viewer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(nacad_viewer_core PUBLIC cxx_std_20)
target_link_libraries(nacad_viewer_core
    PUBLIC glm
    PRIVATE nacad_options nacad_dev_options
)

# -----------------------------------------------------------
# Tests
# -----------------------------------------------------------
if(NACAD_BUILD_TESTS)
    add_executable(nacad_viewer_tests
        tests/test_main.cpp
        tests/test_asset_paths.cpp
        tests/test_camera.cpp
        tests/test_mouse_look.cpp
        tests/test_projection.cpp
    )
    target_link_libraries(nacad_viewer_tests PRIVATE
        nacad_viewer_core
        doctest::doctest
        nacad_options
        nacad_dev_options
    )
    doctest_discover_tests(nacad_viewer_tests)

    if(TARGET coverage)
        add_dependencies(coverage nacad_viewer_tests)
    endif()
endif()

# -----------------------------------------------------------
# NACad executable (GL, GLFW, ImGui, assimp)
# -----------------------------------------------------------
if(NACAD_BUILD_VIEWER)
    add_executable(NACad
        app/main.cpp
        app/App.cpp
        app/Model.cpp
        app/RenderMesh.cpp
        app/ShaderProgram.cpp
        app/Utils.cpp
        app/stb_image.cpp
    )

    target_include_directories(NACad PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${stb_SOURCE_DIR}
        ${nfd_SOURCE_DIR}/include
    )

    target_link_libraries(NACad
        PRIVATE
        glad
        glfw
        assimp
        glm
        imgui
        nfd
        ${OPENGL_LIBRARIES}
        nacad_viewer_core
        nacad_geom
        nacad_options
    )

    if(UNIX)
        target_link_libraries(NACad PRIVATE dl pthread)
    endif()

    # --- Copy shaders, samples and shared libs into the build root (assets are loaded relative to the CWD) ---
    set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
    set(SAMPLES_DIR ${CMAKE_SOURCE_DIR}/samples)
    set(DEST_DIR ${CMAKE_BINARY_DIR})

    add_custom_command(
        TARGET NACad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR} ${DEST_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SAMPLES_DIR} ${DEST_DIR}/samples
        COMMENT "Copying shaders and samples..."
    )

    add_custom_command(
        TARGET NACad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying shared libraries..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:assimp>
        $<TARGET_FILE:glfw>
        ${DEST_DIR}
        VERBATIM
    )
endif()
