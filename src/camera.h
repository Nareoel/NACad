#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {
   public:
    enum class movementType { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };
    Camera();

    float fieldOfView() const;
    glm::mat4 viewMatrix() const;
    const glm::vec3& position() const;
    const glm::vec3& front() const;
    void processKeyboard(movementType movement, double deltaTime);
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
    // the following values may be constants in an unnamed namespace
    const double mouseSensitivity_{0.1};
    const double movementSpeed_{2.5};

    void update_();
    void swap_(Camera& other);
};