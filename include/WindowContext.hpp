#pragma once

#include "GLFW/glfw3.h"

#include <string>
#include <memory>


namespace WGPUBoids {

class WindowContext {
  public:
	WindowContext() = default;
	~WindowContext();

	WindowContext(const WindowContext&) = delete;
	WindowContext& operator=(const WindowContext&) = delete;

	WindowContext(WindowContext&&) noexcept;
	WindowContext& operator=(WindowContext&&) noexcept;

	bool init(int width, int height, const char* title);

	bool shouldClose();
	void pollEvents();



  private:
    GLFWwindow* window;
	int height, width;
	std::string name;
	bool framebufferResized = false;

  public:
	GLFWwindow* getGLFWwindow() const { return window; }
	int getWidth() const { return width; }
	int getHeight() const { return height; }

	bool hasResized() const { return framebufferResized; }
	void setResized(bool resized) { framebufferResized = resized; }

	std::pair<int, int> getFramebufferSize();
};

}; // namespace WGPUBoids