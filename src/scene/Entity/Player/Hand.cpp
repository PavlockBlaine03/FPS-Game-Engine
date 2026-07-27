#include "scene/Entity/Player/Hand.h"
#include "scene/Entity/Weapons/Pistol.h"
#include "rendering/Renderer.h"
#include "rendering/Camera.h"
#include "input/InputManager.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace
{
    constexpr const char* HAND_MODEL_PATH = "assets/models/Body/Hand/Rigged-Hand.fbx";
}

Hand::Hand()
    : m_model(std::make_unique<Model>(HAND_MODEL_PATH))
    , m_gripOffset(-2.29F, 0.29F, 1.34F) // tune to line up with the pistol's grip
    , m_gripRotationDegrees(89.68F, 93.0F, 0.0F)
    // Start in the same ballpark as the pistol's own view scale (0.003F);
    // most downloaded FBX rigs are exported at real-world (cm) scale, so
    // 1.0F here was rendering the hand ~300x too large.
    , m_gripScale(1.7F)
{
}

Hand::~Hand() = default;

void Hand::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light,
    const Pistol& pistol) const
{
    glm::mat4 model = pistol.viewTransform(camera);
    model = glm::translate(model, m_gripOffset);
    model = glm::rotate(model, glm::radians(m_gripRotationDegrees.y), glm::vec3(0.0F, 1.0F, 0.0F));
    model = glm::rotate(model, glm::radians(m_gripRotationDegrees.x), glm::vec3(1.0F, 0.0F, 0.0F));
    model = glm::scale(model, glm::vec3(m_gripScale));

    m_model->render(renderer, shader, camera, model, light);
}

void Hand::debugAdjust(const InputManager& input, const float deltaTime)
{
    const float scaleSpeed = 0.5F * deltaTime;
    const float rotationSpeed = 90.0F * deltaTime;
    const float moveSpeed = 2.0F * deltaTime;

    // Bound to a distinct key set from Pistol::debugAdjust so both can be
    // tuned independently without fighting over the same inputs.
    if (input.isKeyPressed(GLFW_KEY_KP_7)) { m_gripScale *= (1.0F + scaleSpeed); }
    if (input.isKeyPressed(GLFW_KEY_KP_1)) { m_gripScale *= (1.0F - scaleSpeed); }

    if (input.isKeyPressed(GLFW_KEY_J)) { m_gripRotationDegrees.y += rotationSpeed; }
    if (input.isKeyPressed(GLFW_KEY_L)) { m_gripRotationDegrees.y -= rotationSpeed; }
    if (input.isKeyPressed(GLFW_KEY_I)) { m_gripRotationDegrees.x += rotationSpeed; }
    if (input.isKeyPressed(GLFW_KEY_K)) { m_gripRotationDegrees.x -= rotationSpeed; }

    if (input.isKeyPressed(GLFW_KEY_T)) { m_gripOffset.y += moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_G)) { m_gripOffset.y -= moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_F)) { m_gripOffset.x -= moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_H)) { m_gripOffset.x += moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_R)) { m_gripOffset.z -= moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_Y)) { m_gripOffset.z += moveSpeed; }

    if (input.isKeyPressed(GLFW_KEY_KP_5))
    {
        std::cout << "Hand scale=" << m_gripScale
                   << " rotation=(" << m_gripRotationDegrees.x << ", "
                   << m_gripRotationDegrees.y << ")\n"
                   << "Offset=(" << m_gripOffset.x << ", "
                   << m_gripOffset.y << ", " << m_gripOffset.z << ")\n";
    }
}