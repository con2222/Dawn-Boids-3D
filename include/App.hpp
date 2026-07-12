#pragma once

#include "WebGpuContext.hpp"
#include "Renderer.hpp"
#include "WindowContext.hpp"
#include "Camera.hpp"
#include "CoreData.hpp"
#include "Interface.hpp"


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
    float getDeltaTime() const;
    bool handleWindowEvents();

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