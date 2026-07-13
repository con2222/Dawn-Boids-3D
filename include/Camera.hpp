#pragma once

#include "glm/glm.hpp"


namespace WGPUBoids {

class Camera {
  public:
    Camera();

    void update();
    void rotate(float deltaX, float deltaY);
    void zoom(float delta);

    void moveForward(float deltaTime);
    void moveBackward(float deltaTime);
    void moveLeft(float deltaTime);
    void moveRight(float deltaTime);
    void moveUp(float deltaTime);
    void moveDown(float deltaTime);

    void updateCameraSpeed(float delta);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;

    void debug() const;

  private:
    glm::vec3 position;

    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float fov;

  public:
    glm::vec3 getPosition() const { return position; }
};

}; // namespace WGPUBoids