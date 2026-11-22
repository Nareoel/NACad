#pragma once

#include <glm/glm.hpp>

namespace Viewer {

class Camera {
   public:
    enum class MovementType { Forward, Backward, Left, Right, Up, Down };
    Camera();

    float fieldOfView() const;
    double yaw() const;
    double pitch() const;
    glm::mat4 viewMatrix() const;
    const glm::vec3& position() const;
    const glm::vec3& front() const;
    const glm::vec3& up() const;
    void processKeyboard(MovementType movement, double deltaTime);
    void processMouse(double xOffset, double yOffset);
    void processScroll(double yOffset);
    void resetView();

   private:
    glm::vec3 position_{glm::vec3{0.0, 0.0, 3.0}};
    glm::vec3 front_{glm::vec3{0.0, 0.0, -1.0}};
    glm::vec3 up_{glm::vec3{0.0, 1.0, 0.0}};
    const glm::vec3 worldUp_{glm::vec3{0.0, 1.0, 0.0}};
    double yaw_{-90.0};
    double pitch_{0.0};
    double fieldOfView_{45.0};

    void update_();
    void swap_(Camera& other);
};

}  // namespace Viewer
