#pragma once

#include "webgpu/webgpu_cpp.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <CoreData.hpp>

namespace WGPUBoids {

class Camera;

class Interface {
public:
    Interface() = default;
    ~Interface();

    bool init(GLFWwindow* window, Camera* pCamera, wgpu::Device device, wgpu::TextureFormat targetFormat);
    void destroy();
    void beginFrame();
    void buildUI();
    void draw(wgpu::RenderPassEncoder renderPass);
private:
    unsigned int fontSize = 16.f;
    float currentScale = 2.1f;
    SimulationParams params;
    bool showVelocity = false;
    bool showCoM = false;
    bool divideFlocks = false;

    GLFWwindow* glfwWindow = nullptr;
    Camera* camera = nullptr;
    bool showControlPanel = false;
public:
    const SimulationParams& getParams() const { return params; }
    void setDeltaTime(float deltaTime) { params.deltaTime = deltaTime; }

    bool getShowVelocity() const { return showVelocity; }
    bool getShowCoM() const { return showCoM; }
};

} // namespace WGPUBoids