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

bool InputManager::isKeyJustPressed(const int key) const
{
    return key >= 0 && key <= GLFW_KEY_LAST
        && m_keyJustPressed[static_cast<std::size_t>(key)];
}

bool InputManager::isMouseButtonPressed(const int button) const
{
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}

bool InputManager::isMouseButtonJustPressed(const int button) const
{
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST
        && m_mouseJustPressed[static_cast<std::size_t>(button)];
}

bool InputManager::isInteractKeyJustPressed() const
{
    return m_interactKeyJustPressed;
}

void InputManager::update()
{
    constexpr int trackedKeys[] = {
        GLFW_KEY_F1, GLFW_KEY_TAB, GLFW_KEY_G, GLFW_KEY_UP, GLFW_KEY_DOWN,
        GLFW_KEY_ENTER, GLFW_KEY_PAGE_UP, GLFW_KEY_PAGE_DOWN, GLFW_KEY_R,
        GLFW_KEY_S, GLFW_KEY_L, GLFW_KEY_C, GLFW_KEY_T,
        GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET
    };
    for (const int key : trackedKeys)
    {
        const bool pressed = isKeyPressed(key);
        m_keyJustPressed[static_cast<std::size_t>(key)] =
            pressed && !m_keyWasPressed[static_cast<std::size_t>(key)];
        m_keyWasPressed[static_cast<std::size_t>(key)] = pressed;
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
    {
        const bool pressed = isMouseButtonPressed(button);
        m_mouseJustPressed[static_cast<std::size_t>(button)] =
            pressed && !m_mouseWasPressed[static_cast<std::size_t>(button)];
        m_mouseWasPressed[static_cast<std::size_t>(button)] = pressed;
    }

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
