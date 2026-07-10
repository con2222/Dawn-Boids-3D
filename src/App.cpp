#include "App.hpp"
#include "ResourceManager.hpp"

#include <iostream>
#include <random>

void printMatrix(const std::string& name, const glm::mat4& m) {
    std::cout << "=== " << name << " ===" << std::endl;
    for (int row = 0; row < 4; ++row) {
        std::cout << "[ ";
        for (int col = 0; col < 4; ++col) {
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << m[col][row] << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << "==================\n" << std::endl;
}

namespace WGPUBoids {

bool App::init(int width, int height, const char *title) {
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);


    // TODO: new func for random numbers
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution dis(-4.0f, 4.0f);

    for (size_t i = 0; i < numBoids; i++) {
        boids.push_back(BoidData(glm::vec4(dis(gen), dis(gen), dis(gen), 0.f), glm::vec4(dis(gen), dis(gen), dis(gen), 0.f)));
    }

    if (!window.init(width, height, title)) {
        std::cerr << "Failed to initialize Window Context\n";
        return false;
    }

    if (!gpuContext.init(window.getGLFWwindow(), width, height)) {
        std::cerr << "Failed to initialize WebGPU Context\n";
        return false;
    }

    wgpu::ShaderModule shader = ResourceManager::getInstance().loadShaderModule(SHADER_DIR "/boids.wgsl", gpuContext.getDevice());
    if (!shader) {
        std::cerr << "Failed to load shader\n";
        return false;
    }

    if (!renderer.init(gpuContext.getDevice(), gpuContext.getQueue(), shader, gpuContext.getSurfaceFormat(), gpuContext.getDepthTextureFormat(), boids)) {
        std::cerr << "Failed to initialize renderer\n";
        return false;
    }

    renderer.updateMeshBuffers(ResourceManager::getInstance().loadObj("boid.obj"));

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
    renderer.updateBoidsData(boids);
    renderer.draw(gpuContext, camera);
    gpuContext.present();
}

void App::processInput() {
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(deltaTime);
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS) camera.moveBackward(deltaTime);
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft(deltaTime);
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(deltaTime);
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_P) == GLFW_PRESS) {
        printMatrix("\nView Matrix", camera.getViewMatrix());
        printMatrix("Projection", camera.getProjectionMatrix(1920.0f / 1080.0f));
    }

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

float App::getDeltaTime() const {
    static double lastFrame = 0.0;
    double currentFrame = glfwGetTime();
    float deltaTime = static_cast<float>(currentFrame - lastFrame);
    lastFrame = currentFrame;

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