#pragma once

#include "WebGpuContext.hpp"
#include "Renderer.hpp"
#include "WindowContext.hpp"
#include "Camera.hpp"
#include "CoreData.hpp"
#include "Interface.hpp"

#include <chrono>


namespace WGPUBoids {

class App {
  public:
    App() = default;
    ~App() = default;

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App(App&&) noexcept;
    App& operator=(App&&) noexcept;

    bool init(const char* title);

    void run();

  private:
    void update(float deltaTime);
    void render();
    void processInput();
    void handleKeyboard();
    void handleMouse();
    float getDeltaTime() const;
    bool handleWindowEvents();
    void generateInitialBoids();
    void enforceFPSLimit(std::chrono::time_point<std::chrono::high_resolution_clock> frameStart, int targetFPS);

    WindowContext window;
    WebGPUContext gpuContext;
    Renderer renderer;
    Camera camera;
    Interface uiLayer;

    std::vector<BoidData> boids;
    int numBoids = 150000;

    float deltaTime;
};

}; // namespace WGPUBoids