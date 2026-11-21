#pragma once

#include <glm/glm.hpp>

namespace Viewer {

constexpr double cDefaultAspectRatio{4.0 / 3.0};
constexpr float cNearPlane{0.1f};
constexpr float cFarPlane{100.0f};

// returns cDefaultAspectRatio for an empty framebuffer (e.g. a minimised window)
double aspectRatio(int width, int height);
glm::mat4 perspective(double fieldOfViewDeg, double aspect);

}  // namespace Viewer
