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
	float getAndResetScrollDelta();



  private:
    GLFWwindow* window;
	int height, width;
	std::string name;
	bool framebufferResized = false;
	float scrollDelta = 0.f;

	bool isFullscreen = false;
	int savedX = 0, savedY = 0, savedWidth = 0, savedHeight = 0;

  public:
	GLFWwindow* getGLFWwindow() const { return window; }
	int getWidth() const { return width; }
	int getHeight() const { return height; }

	bool hasResized() const { return framebufferResized; }
	void setResized(bool resized) { framebufferResized = resized; }
	void setScrollDelta(float delta) { scrollDelta = delta; }

	void toggleFullscreen();

	std::pair<int, int> getFramebufferSize();
};

}; // namespace WGPUBoids