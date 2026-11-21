#include "core/Projection.h"

#include <doctest/doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

TEST_CASE("aspectRatio divides width by height") {
    CHECK(Viewer::aspectRatio(1600, 1200) == doctest::Approx(4.0 / 3.0).epsilon(1e-12));
    CHECK(Viewer::aspectRatio(800, 600) == doctest::Approx(4.0 / 3.0).epsilon(1e-12));
    CHECK(Viewer::aspectRatio(1920, 1080) == doctest::Approx(16.0 / 9.0).epsilon(1e-12));
    CHECK(Viewer::aspectRatio(1, 1) == doctest::Approx(1.0).epsilon(1e-12));
}

TEST_CASE("aspectRatio of an empty framebuffer falls back to the default") {
    const int sizes[][2] = {{1600, 0}, {0, 0}, {0, 1200}, {-5, 10}, {10, -5}};
    for (const auto& size : sizes) {
        CAPTURE(size[0]);
        CAPTURE(size[1]);
        const double aspect = Viewer::aspectRatio(size[0], size[1]);
        CHECK(std::isfinite(aspect));
        CHECK(aspect == doctest::Approx(Viewer::cDefaultAspectRatio).epsilon(1e-12));
    }
}

TEST_CASE("perspective matches glm::perspective with near 0.1 and far 100") {
    const double params[][2] = {{45.0, 4.0 / 3.0}, {1.0, 16.0 / 9.0}, {30.0, 0.5}};
    for (const auto& [fov, aspect] : params) {
        CAPTURE(fov);
        CAPTURE(aspect);
        const glm::mat4 expected =
            glm::perspective(glm::radians(static_cast<float>(fov)), static_cast<float>(aspect), 0.1f, 100.0f);
        const glm::mat4 actual = Viewer::perspective(fov, aspect);
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                CHECK(actual[col][row] == doctest::Approx(expected[col][row]).epsilon(1e-6));
            }
        }
    }
}

TEST_CASE("perspective maps the near and far planes to NDC depth -1 and 1") {
    const glm::mat4 projection = Viewer::perspective(45.0, 4.0 / 3.0);
    const glm::vec4 nearPoint = projection * glm::vec4(0.0f, 0.0f, -Viewer::cNearPlane, 1.0f);
    const glm::vec4 farPoint = projection * glm::vec4(0.0f, 0.0f, -Viewer::cFarPlane, 1.0f);
    CHECK(nearPoint.z / nearPoint.w == doctest::Approx(-1.0).epsilon(1e-5));
    CHECK(farPoint.z / farPoint.w == doctest::Approx(1.0).epsilon(1e-5));
}
