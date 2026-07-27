#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>

class Window
{
public:
	Window(const int width, const int height, const std::string& title);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	[[nodiscard]] bool shouldClose() const;
	void swapBuffers() const;
	static void pollEvents();

	[[nodiscard]] GLFWwindow* handle() const { return m_window; }

private:
	static void framebufferSizeCallback(
		GLFWwindow* window,
		int width,
		int height
	);

	GLFWwindow* m_window = nullptr;
	int m_width;
	int m_height;
};