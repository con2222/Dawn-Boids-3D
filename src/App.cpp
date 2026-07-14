#include "App.hpp"
#include "ResourceManager.hpp"
#include "imgui.h"
#include "Debug.hpp"

#include <iostream>
#include <random>


namespace WGPUBoids {

bool App::init(const char *title) {
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    int width = mode->width;
    int height = mode->height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    if (!window.init(width, height, title)) {
        std::cerr << "Failed to initialize Window Context\n";
        return false;
    }

    // TODO: new func for random numbers
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution dis(-uiLayer.getParams().cubeSize * 3.f, uiLayer.getParams().cubeSize * 3.f);

    for (size_t i = 0; i < numBoids; i++) {
        float flockId = static_cast<float>(i % 3);
        boids.push_back(BoidData(glm::vec4(dis(gen), dis(gen), dis(gen), flockId), glm::vec4(dis(gen), dis(gen), dis(gen), 0.f), glm::vec4(0.f)));
    }
    
    if (!gpuContext.init(window.getGLFWwindow(), width, height)) {
        std::cerr << "Failed to initialize WebGPU Context\n";
        return false;
    }

    wgpu::ShaderModule renderShader = ResourceManager::getInstance().loadShaderModule(SHADER_DIR "/boids.wgsl", gpuContext.getDevice());
    wgpu::ShaderModule computeShader = ResourceManager::getInstance().loadShaderModule(SHADER_DIR "/compute.wgsl", gpuContext.getDevice());
    if (!renderShader || !computeShader) {
        std::cerr << "Failed to load shaders\n";
        return false;
    }


    if (!renderer.init(gpuContext.getDevice(), gpuContext.getQueue(), renderShader, computeShader, gpuContext.getSurfaceFormat(), gpuContext.getDepthTextureFormat(), boids, uiLayer.getParams())) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }

    renderer.updateMeshBuffers(ResourceManager::getInstance().loadObj("boid.obj"));

    if (!uiLayer.init(window.getGLFWwindow(), &camera, gpuContext.getDevice(), gpuContext.getSurfaceFormat())) {
        std::cerr << "Could not initialize UI Layer!" << std::endl;
        return false;
    }

    return true;
}

void App::run() {
    while (!window.shouldClose()) {
        deltaTime = getDeltaTime();
        handleWindowEvents();
        update(deltaTime);
        render();
    }
}

void App::update(float deltaTime) {
    processInput();
}

void App::render() {
    uiLayer.setDeltaTime(deltaTime);
    uiLayer.beginFrame();
    uiLayer.buildUI();
    renderer.draw(gpuContext, camera, uiLayer);
    gpuContext.present();
}

void App::processInput() {
    ImGuiIO& io = ImGui::GetIO();


    if (!io.WantCaptureKeyboard) {
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(deltaTime);
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS) camera.moveBackward(deltaTime);
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft(deltaTime);
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(deltaTime);
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_SPACE) == GLFW_PRESS) camera.moveUp(deltaTime);
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.moveDown(deltaTime);

        // TODO: Add new params
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_EQUAL) == GLFW_PRESS) camera.updateCameraSpeed(0.05f);
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_MINUS) == GLFW_PRESS) camera.updateCameraSpeed(-0.05f);

        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_P) == GLFW_PRESS) {
            printMatrix("\nView Matrix", camera.getViewMatrix());
            printMatrix("Projection", camera.getProjectionMatrix(1920.0f / 1080.0f));
        }
    }
    

    if (!io.WantCaptureMouse) {
        float scroll = window.getAndResetScrollDelta();
        if (scroll != 0.f) {
            camera.zoom(scroll);
        }

        static double lastX = 0.0, lastY = 0.0;
        static bool isDragging = false;

        if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window.getGLFWwindow(), &xpos, &ypos);

            if (!isDragging) {
                lastX = xpos;
                lastY = ypos;
                isDragging = true;
            }
            else {
                camera.rotate(static_cast<float>(xpos - lastX), static_cast<float>(ypos - lastY));
                lastX = xpos;
                lastY = ypos;
            }
        }
        else {
            isDragging = false;
        }
    }
}

float App::getDeltaTime() const {
    static double lastFrame = 0.0;
    double currentFrame = glfwGetTime();
    float deltaTime = static_cast<float>(currentFrame - lastFrame);
    lastFrame = currentFrame;

    if (deltaTime > 0.1f) {
        deltaTime = 0.016f; 
    }

    return deltaTime;
}

bool App::handleWindowEvents()
{
    window.pollEvents();
    auto [width, height] = window.getFramebufferSize();
    if (width <= 0 || height <= 0) { return false; }

    if (window.hasResized()) {
        gpuContext.resizeSwapchain(width, height);
        window.setResized(false);
        render();
    }

    return true;
}

}; // namespace WGPUBoids