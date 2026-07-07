#include "App.hpp"
#include "ResourceManager.hpp"

#include <iostream>

namespace WGPUBoids {

bool App::init(int width, int height, const char *title) {
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    if (!window.init(width, height, title)) {
        std::cerr << "Failed to initialize Window Context\n";
        return false;
    }

    if (!gpuContext.init(window.getGLFWwindow(), width, height)) {
        std::cerr << "Failed to initialize WebGPU Context\n";
        return false;
    }

    wgpu::ShaderModule shader = ResourceManager::getInstance().loadShaderModule(SHADER_DIR "/shader.wgsl", gpuContext.getDevice());
    if (!shader) {
        std::cerr << "Failed to load shader\n";
        return false;
    }

    if (!renderer.init(gpuContext.getDevice(), gpuContext.getQueue(), shader, gpuContext.getSurfaceFormat(), gpuContext.getDepthTextureFormat())) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }

    renderer.updateMeshBuffers(ResourceManager::getInstance().loadObj("cube.obj"));

    return true;
}

void App::run() {
    while (!window.shouldClose()) {
        window.pollEvents();
        update(0.016f);
        render();
    }
}

void App::update(float deltaTime) {

}

void App::render() {
    renderer.draw(gpuContext);
    gpuContext.present();
}

}; // namespace WGPUBoids