# -----------------------------------------------------------
# nacad_geom: GL-free geometry library (C++20 + Eigen)
# -----------------------------------------------------------
add_library(nacad_geom STATIC
    geom/Version.cpp
)
add_library(nacad::geom ALIAS nacad_geom)

# headers sit next to their sources and are included with the prefix, e.g. "geom/Version.h"
target_include_directories(nacad_geom PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(nacad_geom PUBLIC cxx_std_20)
target_link_libraries(nacad_geom
    PUBLIC Eigen3::Eigen
    PRIVATE nacad_options nacad_dev_options
)

# -----------------------------------------------------------
# Tests
# -----------------------------------------------------------
if(NACAD_BUILD_TESTS)
    add_executable(nacad_geom_tests
        tests/test_main.cpp
        tests/test_smoke.cpp
    )
    target_link_libraries(nacad_geom_tests PRIVATE
        nacad_geom
        doctest::doctest
        nacad_options
        nacad_dev_options
    )
    doctest_discover_tests(nacad_geom_tests)

    if(TARGET coverage)
        add_dependencies(coverage nacad_geom_tests)
    endif()
endif()
