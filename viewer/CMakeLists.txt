# -----------------------------------------------------------
# nacad_viewer_core: GL-free viewer logic (camera, projection, mouse look, asset paths, shader
# source preprocessing), glm only
# -----------------------------------------------------------
add_library(nacad_viewer_core STATIC
    core/AssetPaths.cpp
    core/Camera.cpp
    core/MouseLook.cpp
    core/Projection.cpp
    core/ShaderSource.cpp
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
        tests/test_shader_source.cpp
    )
    target_link_libraries(nacad_viewer_tests PRIVATE
        nacad_viewer_core
        doctest::doctest
        nacad_options
        nacad_dev_options
        nacad_test_options
    )
    doctest_discover_tests(nacad_viewer_tests)

    if(TARGET coverage)
        add_dependencies(coverage nacad_viewer_tests)
    endif()
endif()

# -----------------------------------------------------------
# NACad executable (GL, GLFW, ImGui; assimp and nfd on the desktop only)
# -----------------------------------------------------------
if(NACAD_BUILD_VIEWER)
    add_executable(NACad
        app/main.cpp
        app/App.cpp
        app/RenderMesh.cpp
        app/ShaderProgram.cpp
        app/Utils.cpp
        app/stb_image.cpp
    )

    target_include_directories(NACad PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${stb_SOURCE_DIR}
    )

    target_link_libraries(NACad
        PRIVATE
        glfw
        glm
        imgui
        nacad_viewer_core
        nacad_geom
        nacad_options
    )

    if(EMSCRIPTEN)
        # the page is NACad.html + .js + .wasm, and .data holds the preloaded shaders/ and container images
        set(NACAD_WEB_SHELL ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
        set_target_properties(NACad PROPERTIES SUFFIX ".html")
        target_link_options(NACad PRIVATE
            -sMIN_WEBGL_VERSION=2
            -sMAX_WEBGL_VERSION=2
            -sALLOW_MEMORY_GROWTH=1
            "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders@/shaders"
            "SHELL:--shell-file ${NACAD_WEB_SHELL}"
        )
        set(NACAD_WEB_LINK_DEPENDS
            ${NACAD_WEB_SHELL}
            ${CMAKE_SOURCE_DIR}/shaders/shader.vs
            ${CMAKE_SOURCE_DIR}/shaders/shader.fs
            ${CMAKE_SOURCE_DIR}/shaders/shaderLightSource.fs
        )
        # samples/ is gitignored, so only the container images that exist at configure time are packed
        foreach(sampleImage container.png containerMetalBorder.png matrix.jpg)
            set(sampleImagePath ${CMAKE_SOURCE_DIR}/samples/${sampleImage})
            if(EXISTS ${sampleImagePath})
                target_link_options(NACad PRIVATE "SHELL:--preload-file ${sampleImagePath}@/samples/${sampleImage}")
                list(APPEND NACAD_WEB_LINK_DEPENDS ${sampleImagePath})
            endif()
        endforeach()
        set_property(TARGET NACad APPEND PROPERTY LINK_DEPENDS ${NACAD_WEB_LINK_DEPENDS})
    else()
        target_sources(NACad PRIVATE app/Model.cpp)
        target_include_directories(NACad PRIVATE ${nfd_SOURCE_DIR}/include)
        target_link_libraries(NACad PRIVATE glad assimp nfd ${OPENGL_LIBRARIES})

        if(UNIX)
            target_link_libraries(NACad PRIVATE dl pthread)
        endif()

        # --- Copy shaders, samples and shared libs into the build root (App finds them next to the executable) ---
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
endif()
