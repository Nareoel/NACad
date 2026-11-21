#include "core/Camera.h"

#include <doctest/doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {
constexpr double cEps{1e-5};

using Viewer::Camera;

void sCheckVec(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(cEps));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(cEps));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(cEps));
}

void sCheckOrthonormal(const Camera& camera) {
    CHECK(glm::length(camera.front()) == doctest::Approx(1.0).epsilon(cEps));
    CHECK(glm::length(camera.up()) == doctest::Approx(1.0).epsilon(cEps));
    CHECK(std::abs(glm::dot(camera.front(), camera.up())) < cEps);
}

void sCheckDefaults(const Camera& camera) {
    sCheckVec(camera.position(), {0.0f, 0.0f, 3.0f});
    sCheckVec(camera.front(), {0.0f, 0.0f, -1.0f});
    sCheckVec(camera.up(), {0.0f, 1.0f, 0.0f});
    CHECK(camera.yaw() == doctest::Approx(-90.0));
    CHECK(camera.pitch() == doctest::Approx(0.0));
    CHECK(camera.fieldOfView() == doctest::Approx(45.0));
}

}  // namespace

TEST_CASE("camera default state looks down -z from (0, 0, 3)") {
    const Camera camera;
    sCheckDefaults(camera);
    sCheckOrthonormal(camera);
}

TEST_CASE("camera view matrix is lookAt from position along front") {
    const Camera camera;
    const glm::mat4 expected =
        glm::lookAt(camera.position(), camera.position() + camera.front(), camera.up());
    const glm::mat4 actual = camera.viewMatrix();
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            CHECK(actual[col][row] == doctest::Approx(expected[col][row]).epsilon(cEps));
        }
    }
    const glm::vec4 eye = actual * glm::vec4(camera.position(), 1.0f);
    sCheckVec(glm::vec3(eye), {0.0f, 0.0f, 0.0f});
}

TEST_CASE("camera keyboard movement moves by speed * deltaTime in the right direction") {
    constexpr double cDeltaTime{0.4};  // 2.5 * 0.4 = 1 unit
    struct Case {
        Camera::MovementType movement;
        glm::vec3 expected;
    };
    const Case cases[] = {
        {Camera::MovementType::Forward, {0.0f, 0.0f, 2.0f}},
        {Camera::MovementType::Backward, {0.0f, 0.0f, 4.0f}},
        {Camera::MovementType::Right, {1.0f, 0.0f, 3.0f}},
        {Camera::MovementType::Left, {-1.0f, 0.0f, 3.0f}},
        {Camera::MovementType::Up, {0.0f, 1.0f, 3.0f}},
        {Camera::MovementType::Down, {0.0f, -1.0f, 3.0f}},
    };
    for (const auto& [movement, expected] : cases) {
        CAPTURE(static_cast<int>(movement));
        Camera camera;
        camera.processKeyboard(movement, cDeltaTime);
        sCheckVec(camera.position(), expected);
        sCheckVec(camera.front(), {0.0f, 0.0f, -1.0f});
    }
}

TEST_CASE("camera keyboard movement with zero deltaTime does not move") {
    Camera camera;
    camera.processKeyboard(Camera::MovementType::Forward, 0.0);
    sCheckVec(camera.position(), {0.0f, 0.0f, 3.0f});
}

TEST_CASE("camera mouse yaw of 90 degrees turns front from -z to +x") {
    Camera camera;
    camera.processMouse(900.0, 0.0);  // sensitivity 0.1
    CHECK(camera.yaw() == doctest::Approx(0.0));
    sCheckVec(camera.front(), {1.0f, 0.0f, 0.0f});
    sCheckVec(camera.up(), {0.0f, 1.0f, 0.0f});
    sCheckOrthonormal(camera);
}

TEST_CASE("camera mouse y offset increases pitch") {
    Camera camera;
    camera.processMouse(0.0, 300.0);
    CHECK(camera.pitch() == doctest::Approx(30.0));
    const double pitch = glm::radians(30.0);
    sCheckVec(camera.front(),
              {0.0f, static_cast<float>(std::sin(pitch)), static_cast<float>(-std::cos(pitch))});
    sCheckOrthonormal(camera);
}

TEST_CASE("camera pitch is clamped to +-89 degrees") {
    Camera camera;
    camera.processMouse(0.0, 100000.0);
    CHECK(camera.pitch() == doctest::Approx(89.0));
    CHECK(camera.front().y == doctest::Approx(std::sin(glm::radians(89.0))).epsilon(cEps));
    sCheckOrthonormal(camera);

    camera.processMouse(0.0, -100000.0);
    CHECK(camera.pitch() == doctest::Approx(-89.0));
    CHECK(camera.front().y == doctest::Approx(std::sin(glm::radians(-89.0))).epsilon(cEps));
    sCheckOrthonormal(camera);
}

TEST_CASE("camera scroll zooms and clamps the field of view to [1, 45]") {
    Camera camera;
    camera.processScroll(5.0);
    CHECK(camera.fieldOfView() == doctest::Approx(40.0));
    camera.processScroll(-2.5);
    CHECK(camera.fieldOfView() == doctest::Approx(42.5));
    camera.processScroll(100.0);
    CHECK(camera.fieldOfView() == doctest::Approx(1.0));
    camera.processScroll(-100.0);
    CHECK(camera.fieldOfView() == doctest::Approx(45.0));
}

TEST_CASE("camera resetView restores the defaults and the camera stays usable") {
    Camera camera;
    camera.processMouse(123.0, -45.0);
    camera.processScroll(10.0);
    camera.processKeyboard(Camera::MovementType::Forward, 1.0);
    camera.processKeyboard(Camera::MovementType::Up, 1.0);

    camera.resetView();
    sCheckDefaults(camera);

    camera.processKeyboard(Camera::MovementType::Forward, 0.4);
    sCheckVec(camera.position(), {0.0f, 0.0f, 2.0f});
    camera.processMouse(900.0, 0.0);
    sCheckVec(camera.front(), {1.0f, 0.0f, 0.0f});
}

TEST_CASE("camera front and up stay orthonormal after mixed input") {
    Camera camera;
    const double offsets[][2] = {{10.0, 5.0}, {-300.0, 120.0}, {45.0, -800.0}, {1234.0, 3.0}, {-7.5, 2000.0}};
    for (const auto& offset : offsets) {
        camera.processMouse(offset[0], offset[1]);
        camera.processKeyboard(Camera::MovementType::Right, 0.1);
        camera.processKeyboard(Camera::MovementType::Up, 0.1);
        sCheckOrthonormal(camera);
    }
}
