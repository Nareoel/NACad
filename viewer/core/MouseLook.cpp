#include "core/MouseLook.h"

namespace Viewer {

std::optional<glm::dvec2> MouseLook::delta(double xPos, double yPos) {
    const glm::dvec2 pos{xPos, yPos};
    std::optional<glm::dvec2> result;
    if (lastPos_) {
        result = pos - *lastPos_;
    }
    lastPos_ = pos;
    return result;
}

void MouseLook::reset() { lastPos_.reset(); }

}  // namespace Viewer
