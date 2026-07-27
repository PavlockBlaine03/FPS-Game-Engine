#pragma once

#include <GLFW/glfw3.h>

class InputManager
{
public:
	explicit InputManager(GLFWwindow* window);

	[[nodiscard]] bool isKeyPressed(const int key) const;
	[[nodiscard]] bool isMouseButtonPressed(int button) const;

	// True only on the frame the button transitions from released to
	// pressed (i.e. a single click), unlike isMouseButtonPressed which
	// stays true the whole time it's held. Call update() once per frame
	// (already done via resetMouseDelta's frame boundary) to track this.
	[[nodiscard]] bool isMouseButtonJustPressed(int button) const;

	// Same edge-detection idea as isMouseButtonJustPressed, but for the
	// interact key (E), so holding it down doesn't repeatedly toggle
	// whatever it's used to interact with (e.g. a door).
	[[nodiscard]] bool isInteractKeyJustPressed() const;

	void update();

	[[nodiscard]] float mouseDeltaX() const { return m_mouseDeltaX; }
	[[nodiscard]] float mouseDeltaY() const { return m_mouseDeltaY; }

	void resetMouseDelta();

private:
	static void cursorPositionCallback(
		GLFWwindow* window,
		double xPosition,
		double yPosition);

	GLFWwindow* m_window = nullptr;

	float m_lastMouseX = 0.0f;
	float m_lastMouseY = 0.0f;
	float m_mouseDeltaX = 0.0f;
	float m_mouseDeltaY = 0.0f;
	bool m_firstMouse = true;

	bool m_leftMouseWasPressed = false;
	bool m_leftMouseJustPressed = false;

	bool m_interactKeyWasPressed = false;
	bool m_interactKeyJustPressed = false;
};