#include "core/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

namespace {
constexpr double cMouseSensitivity{0.1};
constexpr double cMovementSpeed{2.5};
constexpr double cPitchLimit{89.0};
constexpr double cMinFieldOfView{1.0};
constexpr double cMaxFieldOfView{45.0};
}  // namespace

namespace Viewer {

Camera::Camera() { update_(); }
float Camera::fieldOfView() const { return static_cast<float>(fieldOfView_); }
double Camera::yaw() const { return yaw_; }
double Camera::pitch() const { return pitch_; }
glm::mat4 Camera::viewMatrix() const { return glm::lookAt(position_, position_ + front_, up_); }
const glm::vec3& Camera::position() const { return position_; }
const glm::vec3& Camera::front() const { return front_; }
const glm::vec3& Camera::up() const { return up_; }
void Camera::processKeyboard(MovementType movement, double deltaTime) {
    const auto velocity = static_cast<float>(cMovementSpeed * deltaTime);
    switch (movement) {
        case Camera::MovementType::Backward:
            position_ -= velocity * front_;
            break;
        case Camera::MovementType::Forward:
            position_ += velocity * front_;
            break;
        case Camera::MovementType::Left:
            position_ -= glm::normalize(glm::cross(front_, up_)) * velocity;
            break;
        case Camera::MovementType::Right:
            position_ += glm::normalize(glm::cross(front_, up_)) * velocity;
            break;
        case Camera::MovementType::Down:
            position_ -= up_ * velocity;
            break;
        case Camera::MovementType::Up:
            position_ += up_ * velocity;
            break;
        default:
            assert(false);
            break;
    }
    update_();
}
void Camera::processMouse(double xOffset, double yOffset) {
    yaw_ += xOffset * cMouseSensitivity;
    pitch_ += yOffset * cMouseSensitivity;
    pitch_ = std::clamp(pitch_, -cPitchLimit, cPitchLimit);
    update_();
}
void Camera::processScroll(double yOffset) {
    fieldOfView_ -= yOffset;
    fieldOfView_ = std::clamp(fieldOfView_, cMinFieldOfView, cMaxFieldOfView);
    update_();
}
void Camera::resetView() {
    auto camera = Camera();
    swap_(camera);
}
void Camera::update_() {
    const auto yaw = glm::radians(yaw_);
    const auto pitch = glm::radians(pitch_);
    front_ = glm::normalize(
        glm::vec3(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch)));
    const auto right = glm::normalize(glm::cross(front_, worldUp_));
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

}  // namespace Viewer
