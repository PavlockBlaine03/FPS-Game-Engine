#include "rendering/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

Camera::Camera(const glm::vec3& position, const float yaw, const float pitch)
    : m_position(position)
    , m_yaw(yaw)
    , m_pitch(pitch)
{
    updateVectors();
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::projectionMatrix(const float aspectRatio) const
{
    return glm::perspective(
        glm::radians(m_fieldOfView),
        aspectRatio,
        0.1F,
        100.0F
    );
}

void Camera::processKeyboard(const glm::vec3& direction, const float deltaTime)
{
    const float velocity = m_movementSpeed * deltaTime;

    m_position += m_front * direction.z * velocity;
    m_position += m_right * direction.x * velocity;
    m_position += m_worldUp * direction.y * velocity;
}

void Camera::processMouseMovement(
    const float xOffset,
    const float yOffset,
    const bool constrainPitch)
{
    m_yaw += xOffset * m_mouseSensitivity;
    m_pitch += yOffset * m_mouseSensitivity;

    if (constrainPitch)
    {
        m_pitch = std::clamp(m_pitch, -89.0F, 89.0F);
    }

    updateVectors();
}

void Camera::processMouseScroll(const float yOffset)
{
    m_fieldOfView = std::clamp(m_fieldOfView - yOffset, 1.0F, 90.0F);
}

void Camera::updateVectors()
{
    const glm::vec3 direction{
        std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch)),
        std::sin(glm::radians(m_pitch)),
        std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch))
    };

    m_front = glm::normalize(direction);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}