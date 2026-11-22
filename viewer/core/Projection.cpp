#include "core/Projection.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Viewer {

double aspectRatio(int width, int height) {
    if (width <= 0 || height <= 0) {
        return cDefaultAspectRatio;
    }
    return static_cast<double>(width) / static_cast<double>(height);
}

glm::mat4 perspective(double fieldOfViewDeg, double aspect) {
    return glm::perspective(glm::radians(static_cast<float>(fieldOfViewDeg)), static_cast<float>(aspect),
                            cNearPlane, cFarPlane);
}

}  // namespace Viewer
