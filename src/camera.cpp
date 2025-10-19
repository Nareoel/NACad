#include "camera.h"

#include <algorithm>

namespace {}  // namespace
Camera::Camera() { update_(); }
float Camera::fieldOfView() const { return fieldOfView_; }
glm::mat4 Camera::viewMatrix() const { return glm::lookAt(position_, position_ + front_, up_); }
const glm::vec3& Camera::position() const { return position_; }
const glm::vec3& Camera::front() const { return front_; }
void Camera::processKeyboard(movementType movement, double deltaTime) {
    float velocity = movementSpeed_ * deltaTime;
    switch (movement) {
        case Camera::movementType::BACKWARD:
            position_ -= velocity * front_;
            break;
        case Camera::movementType::FORWARD:
            position_ += velocity * front_;
            break;
        case Camera::movementType::LEFT:
            position_ -= glm::normalize(glm::cross(front_, up_)) * velocity;
            break;
        case Camera::movementType::RIGHT:
            position_ += glm::normalize(glm::cross(front_, up_)) * velocity;
            break;
        case Camera::movementType::DOWN:
            position_ -= up_ * velocity;
            break;
        case Camera::movementType::UP:
            position_ += up_ * velocity;
            break;
        default:
            assert(false);
            break;
    }
    update_();
}
void Camera::processMouse(double xOffset, double yOffset) {
    constexpr float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw_ += xOffset;
    pitch_ += yOffset;

    double pitchUpperConstrain{89.0f};
    double pitchLowerConstrain{-89.0f};
    pitch_ = std::clamp(pitch_, pitchLowerConstrain, pitchUpperConstrain);

    update_();
}
void Camera::processScroll(double yOffset) {
    fieldOfView_ -= static_cast<float>(yOffset);
    constexpr double fowUpperConstrain{45.0f};
    constexpr double fowLowerConstrain{1.0f};
    fieldOfView_ = std::clamp(fieldOfView_, fowLowerConstrain, fowUpperConstrain);
    update_();
}
void Camera::resetView() {
    auto camera = Camera();
    swap_(camera);
}
void Camera::update_() {
    front_.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_.y = sin(glm::radians(pitch_));
    front_.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(front_);
    // also re-calculate the Right and Up vector
    auto right = glm::normalize(glm::cross(front_, worldUp_));
    up_ = glm::normalize(glm::cross(right, front_));
}
void Camera::swap_(Camera& other) {
    std::swap(position_, other.position_);
    std::swap(front_, other.front_);
    std::swap(up_, other.up_);
    std::swap(yaw_, other.yaw_);
    std::swap(pitch_, other.pitch_);
    std::swap(fieldOfView_, other.fieldOfView_);
}
