#include "core/Window.h"
#include <iostream>

Window::Window(const int width, const int height, const std::string& title) 
	: m_width(width), m_height(height)
{
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(
		width,
		height,
		title.c_str(),
		nullptr,
		nullptr
	);

	if (m_window == nullptr) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(m_window);
	glfwSetWindowUserPointer(m_window, this);

	glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

	if (!gladLoadGLLoader(
		reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		throw std::runtime_error("Failed to initialize GLAD.");
	}

	glViewport(0, 0, width, height);
}
Window::~Window()
{
	if (m_window != nullptr)
	{
		glfwDestroyWindow(m_window);
	}

	glfwTerminate();
}

bool Window::shouldClose() const
{
	return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

void Window::swapBuffers() const
{
	glfwSwapBuffers(m_window);
}

void Window::pollEvents()
{
	glfwPollEvents();
}

void Window::framebufferSizeCallback(
	GLFWwindow* window,
	const int width,
	const int height)
{
	static_cast<void>(window);
	glViewport(0, 0, width, height);
}