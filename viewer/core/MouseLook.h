#pragma once

#include <optional>
#include <glm/glm.hpp>

namespace Viewer {

// turns absolute cursor positions into offsets; the first event only records the position, so the
// camera does not jump when the cursor enters the window
class MouseLook {
   public:
    std::optional<glm::dvec2> delta(double xPos, double yPos);
    // forget the last position, e.g. when the cursor was used for something else in between
    void reset();

   private:
    std::optional<glm::dvec2> lastPos_;
};

}  // namespace Viewer
