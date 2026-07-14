#include "WindowContext.hpp"
#include "std_image.h"

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

	
	GLFWimage images[1];
	images[0].pixels = stbi_load("assets/app_icon.png", &images[0].width, &images[0].height, 0, 4);
	if (images[0].pixels) {
		glfwSetWindowIcon(window, 1, images);
		stbi_image_free(images[0].pixels);
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