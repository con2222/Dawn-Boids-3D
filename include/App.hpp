#pragma once

#include "WebGpuContext.hpp"
#include "Renderer.hpp"
#include "WindowContext.hpp"
#include "Camera.hpp"
#include "CoreData.hpp"


namespace WGPUBoids {

class App {
  public:
    App() = default;
    ~App() = default;

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App(App&&) noexcept;
    App& operator=(App&&) noexcept;

    bool init(int width, int height, const char* title);

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

    std::vector<BoidData> boids;
    int numBoids = 1000;


    float deltaTime;
};

}; // namespace WGPUBoids