#pragma once

#include "WebGpuContext.hpp"
#include "Renderer.hpp"
#include "WindowContext.hpp"


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

    WindowContext window;
    WebGPUContext gpuContext;
    Renderer renderer;
};

}; // namespace WGPUBoids