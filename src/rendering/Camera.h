#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera(
        const glm::vec3& position = glm::vec3(0.0F, 0.0F, 3.0F),
        float yaw = -90.0F,
        float pitch = 0.0F);

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio) const;

    void processKeyboard(
        const glm::vec3& direction,
        float deltaTime);

    void processMouseMovement(
        float xOffset,
        float yOffset,
        bool constrainPitch = true);

    void processMouseScroll(float yOffset);

    void setPosition(const glm::vec3& position) { m_position = position; }

    [[nodiscard]] const glm::vec3& position() const { return m_position; }
    [[nodiscard]] const glm::vec3& front() const { return m_front; }
    [[nodiscard]] const glm::vec3& right() const { return m_right; }

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front{};
    glm::vec3 m_up{};
    glm::vec3 m_right{};
    glm::vec3 m_worldUp{ 0.0F, 1.0F, 0.0F };

    float m_yaw;
    float m_pitch;

    float m_movementSpeed = 3.0F;
    float m_mouseSensitivity = 0.1F;
    float m_fieldOfView = 45.0F;
};