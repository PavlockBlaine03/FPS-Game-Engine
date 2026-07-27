#include "core/Time.h"
#include <GLFW/glfw3.h>
#include <algorithm>

void Time::update()
{
	const auto currentFrameTime = static_cast<float>(glfwGetTime());
	m_deltaTime = std::min(currentFrameTime - m_lastFrameTime, MAX_DELTA_TIME);
	m_lastFrameTime = currentFrameTime;
}