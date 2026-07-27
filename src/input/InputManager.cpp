#include "input/InputManager.h"

InputManager::InputManager(GLFWwindow* window)
    : m_window(window)
{
    glfwSetWindowUserPointer(m_window, this);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(m_window, cursorPositionCallback);
}

bool InputManager::isKeyPressed(const int key) const
{
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

bool InputManager::isMouseButtonPressed(const int button) const
{
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}

bool InputManager::isMouseButtonJustPressed(const int button) const
{
    // Only the left button is tracked for "just pressed" edge-detection
    // right now, since that's all firing needs; extend with per-button
    // state if other buttons need this later.
    return button == GLFW_MOUSE_BUTTON_LEFT && m_leftMouseJustPressed;
}

bool InputManager::isInteractKeyJustPressed() const
{
    return m_interactKeyJustPressed;
}

void InputManager::update()
{
    const bool isLeftMousePressedNow = isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

    m_leftMouseJustPressed = isLeftMousePressedNow && !m_leftMouseWasPressed;
    m_leftMouseWasPressed = isLeftMousePressedNow;

    const bool isInteractPressedNow = isKeyPressed(GLFW_KEY_E);

    m_interactKeyJustPressed = isInteractPressedNow && !m_interactKeyWasPressed;
    m_interactKeyWasPressed = isInteractPressedNow;
}

void InputManager::resetMouseDelta()
{
    m_mouseDeltaX = 0.0F;
    m_mouseDeltaY = 0.0F;
}

void InputManager::cursorPositionCallback(
    GLFWwindow* window,
    const double xPosition,
    const double yPosition)
{
    auto* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

    if (input == nullptr)
    {
        return;
    }

    const auto xPos = static_cast<float>(xPosition);
    const auto yPos = static_cast<float>(yPosition);

    if (input->m_firstMouse)
    {
        input->m_lastMouseX = xPos;
        input->m_lastMouseY = yPos;
        input->m_firstMouse = false;
    }

    input->m_mouseDeltaX = xPos - input->m_lastMouseX;
    input->m_mouseDeltaY = input->m_lastMouseY - yPos; // reversed: y-coords go top to bottom

    input->m_lastMouseX = xPos;
    input->m_lastMouseY = yPos;
}