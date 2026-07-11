#pragma once

#include "webgpu/webgpu_cpp.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

// std
#include <CoreData.hpp>

namespace WGPUBoids {

class Interface {
public:
    Interface() = default;
    ~Interface();

    bool init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat targetFormat);
    void destroy();
    void beginFrame();
    void buildUI();
    void draw(wgpu::RenderPassEncoder renderPass);
private:
    unsigned int fontSize = 16.f;
    float currentScale = 2.1f;
    SimulationParams params;
public:
    const SimulationParams& getParams() const { return params; }
    void setDeltaTime(float deltaTime) { params.deltaTime = deltaTime; }
};

} // namespace WGPUBoids