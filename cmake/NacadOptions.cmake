# -----------------------------------------------------------
# Shared compile options for NACad targets
# -----------------------------------------------------------
# nacad_options     : warnings, linked by every NACad target (viewer, geom, tests).
# nacad_dev_options : coverage and sanitizers, linked only by the GL-free libraries (nacad_geom,
#                     nacad_viewer_core) and their tests (GL drivers produce too much ASan noise
#                     for the viewer executable). Native builds only.
# nacad_test_options: linked by the test executables; empty on native builds, makes them runnable
#                     under Node in Emscripten builds.

set(NACAD_GCC_OR_CLANG "$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>")

# --- Warnings ---
add_library(nacad_options INTERFACE)
target_compile_options(nacad_options INTERFACE
    $<${NACAD_GCC_OR_CLANG}:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# --- Coverage and sanitizers ---
add_library(nacad_dev_options INTERFACE)

if(EMSCRIPTEN)
    if(NACAD_COVERAGE)
        message(WARNING "NACAD_COVERAGE is not supported in Emscripten builds, ignoring it")
        set(NACAD_COVERAGE OFF)
    endif()
    if(NACAD_SANITIZE)
        message(WARNING "NACAD_SANITIZE is not supported in Emscripten builds, ignoring it")
        set(NACAD_SANITIZE OFF)
    endif()
endif()

if(NACAD_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(nacad_dev_options INTERFACE --coverage -O0 -g)
        target_link_options(nacad_dev_options INTERFACE --coverage)
    else()
        message(WARNING "NACAD_COVERAGE is only supported with GCC/Clang, ignoring it")
    endif()
endif()

if(NACAD_SANITIZE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(nacad_dev_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(nacad_dev_options INTERFACE -fsanitize=address,undefined)
    else()
        message(WARNING "NACAD_SANITIZE is only supported with GCC/Clang, ignoring it")
    endif()
endif()

# --- Test executables ---
add_library(nacad_test_options INTERFACE)
if(EMSCRIPTEN)
    # colour codes would end up in the test names doctest_discover_tests reads from Node's output
    target_compile_definitions(nacad_test_options INTERFACE DOCTEST_CONFIG_COLORS_NONE)
    # doctest's TEST_CASE expands to __COUNTER__ in our files, which emcc's newer Clang flags under -Wpedantic
    target_compile_options(nacad_test_options INTERFACE -Wno-c2y-extensions)
    # EXIT_RUNTIME makes Node exit with main's return value, so failing tests fail in CTest
    target_link_options(nacad_test_options INTERFACE -sEXIT_RUNTIME=1 -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=node)
endif()

# --- Coverage report (gcovr) ---
if(NACAD_COVERAGE)
    find_program(GCOVR gcovr)
    if(GCOVR)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
            COMMAND ${GCOVR} -r ${CMAKE_SOURCE_DIR} --object-directory ${CMAKE_BINARY_DIR}
                    --filter ${CMAKE_SOURCE_DIR}/geom/geom/
                    --filter ${CMAKE_SOURCE_DIR}/viewer/core/
                    --html-details ${CMAKE_BINARY_DIR}/coverage/index.html --print-summary
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests and generating coverage report in ${CMAKE_BINARY_DIR}/coverage"
            VERBATIM
        )
    else()
        message(STATUS "gcovr not found: the 'coverage' target is unavailable (install it with `pip install gcovr`)")
    endif()
endif()
