#pragma once

#include "glm/glm.hpp"


namespace WGPUBoids {

enum class CameraMode {
    Free,
    Orbital
};

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

    CameraMode mode = CameraMode::Free;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    float radius = 10.0f;
    float zoomFactor = 0.5f;

  public:
    glm::vec3 getPosition() const { return position; }

    void setMode(CameraMode newMode);
    CameraMode getMode() const { return mode; }
    
    void setTarget(const glm::vec3& newTarget) { target = newTarget; }

    float getMovementSpeed() const { return movementSpeed; }
    void setMovementSpeed(float speed) { movementSpeed = speed; }

    float getRadius() const { return radius; }
    void setRadius(float r) { 
        radius = r; 
        update();
    }
};

}; // namespace WGPUBoids