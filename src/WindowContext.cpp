#include "WindowContext.hpp"

#include <iostream>

namespace WGPUBoids {

bool WindowContext::init(int width, int height, const char* title) {
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	this->height = height;
	this->width = width;
	this->name = title;

	if (!window) {
		std::cerr << "Can't create window" << std::endl;
		return false;
	}

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(
		window,
		[](GLFWwindow* window, int, int) {
			auto that = reinterpret_cast<WindowContext*>(glfwGetWindowUserPointer(window));
			if (that != nullptr) that->setResized(true);
		}
	);
	glfwSetScrollCallback(
		window,
		[](GLFWwindow* window, double xoffset, double yoffset) {
			auto that = reinterpret_cast<WindowContext*>(glfwGetWindowUserPointer(window));
			if (that != nullptr) {
				that->setScrollDelta(static_cast<float>(yoffset));
			}
		}
	);
	return true;
}


void WindowContext::pollEvents() {
	glfwPollEvents();
}

bool WindowContext::shouldClose() {
	return glfwWindowShouldClose(window);
}

std::pair<int, int> WindowContext::getFramebufferSize() {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return std::pair<int, int>(width, height);
}

WindowContext::~WindowContext() {
	glfwDestroyWindow(window);
}

float WindowContext::getAndResetScrollDelta() {
	float d = scrollDelta;
	scrollDelta = 0.f;
	return d;
}

} // namespace WGPUBoids