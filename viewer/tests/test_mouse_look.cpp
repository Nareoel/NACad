#include "core/MouseLook.h"

#include <doctest/doctest.h>

TEST_CASE("mouse look first event gives no delta") {
    Viewer::MouseLook mouseLook;
    CHECK_FALSE(mouseLook.delta(400.0, 300.0).has_value());
}

TEST_CASE("mouse look later events give the offset from the previous position") {
    Viewer::MouseLook mouseLook;
    REQUIRE_FALSE(mouseLook.delta(400.0, 300.0).has_value());

    const auto first = mouseLook.delta(410.5, 290.0);
    REQUIRE(first.has_value());
    CHECK(first->x == doctest::Approx(10.5));
    CHECK(first->y == doctest::Approx(-10.0));

    const auto second = mouseLook.delta(410.5, 290.0);
    REQUIRE(second.has_value());
    CHECK(second->x == doctest::Approx(0.0));
    CHECK(second->y == doctest::Approx(0.0));

    const auto third = mouseLook.delta(0.0, 1000.0);
    REQUIRE(third.has_value());
    CHECK(third->x == doctest::Approx(-410.5));
    CHECK(third->y == doctest::Approx(710.0));
}

TEST_CASE("mouse look reset makes the next event a first event") {
    // regression: leaving interactive mode used to jump the camera by the cursor travel in that mode
    Viewer::MouseLook mouseLook;
    REQUIRE_FALSE(mouseLook.delta(800.0, 600.0).has_value());

    mouseLook.reset();
    CHECK_FALSE(mouseLook.delta(100.0, 50.0).has_value());

    const auto delta = mouseLook.delta(103.0, 48.0);
    REQUIRE(delta.has_value());
    CHECK(delta->x == doctest::Approx(3.0));
    CHECK(delta->y == doctest::Approx(-2.0));
}

TEST_CASE("mouse look instances are independent") {
    Viewer::MouseLook a;
    Viewer::MouseLook b;
    REQUIRE_FALSE(a.delta(0.0, 0.0).has_value());
    CHECK_FALSE(b.delta(5.0, 5.0).has_value());
    const auto delta = a.delta(1.0, 2.0);
    REQUIRE(delta.has_value());
    CHECK(delta->x == doctest::Approx(1.0));
    CHECK(delta->y == doctest::Approx(2.0));
}
