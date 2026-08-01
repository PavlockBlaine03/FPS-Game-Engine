#include "scene/Entity/Player/Hand.h"
#include "scene/Entity/Weapons/Pistol.h"
#include "rendering/Renderer.h"
#include "rendering/Camera.h"
#include "input/InputManager.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>

namespace
{
    constexpr const char* HAND_MODEL_PATH = "assets/models/Body/Hand/hand.glb";
    constexpr const char* GRIP_ANIMATION = "PistolGrip";
}

Hand::Hand()
    : m_model(std::make_unique<SkeletalModel>(HAND_MODEL_PATH))
    , m_animator(std::make_unique<Animator>(*m_model))
    , m_gripOffset(-2.55947F, 0.261407F, 1.91F) // tuned to line up with the pistol's grip
    , m_gripRotationDegrees(88.7286F, 97.3976F, 0.0F)
    // Start in the same ballpark as the pistol's own view scale (0.003F);
    // most downloaded FBX rigs are exported at real-world (cm) scale, so
    // 1.0F here was rendering the hand ~300x too large.
    , m_gripScale(1.95F)
{
    // This action also contains positioning keys for the source arm rig.
    // The viewmodel transform owns wrist placement, so keep the arm chain in
    // its bind pose and apply the animation only to the palm and digits.
    m_animator->ignoreNodeAnimation("Armature");
    m_animator->ignoreNodeAnimation("shoulder.R");
    m_animator->ignoreNodeAnimation("upper_arm.R");
    m_animator->ignoreNodeAnimation("forearm.R");
    m_animator->ignoreNodeAnimation("forearm.R.003");
    m_animator->ignoreNodeAnimation("forearm.R.003_end");
    m_animator->ignoreNodeAnimation("hand.R");

    // Play once and hold the last frame, which is the authored closed-hand
    // pose. Keeping this in the animator also lets us add draw/reload poses
    // later without changing the rendering path again.
    if (!m_animator->play(GRIP_ANIMATION, false, 0.0F))
    {
        throw std::runtime_error(
            std::string("Hand model requires animation clip: ") + GRIP_ANIMATION);
    }
}

Hand::~Hand() = default;

void Hand::update(const float deltaTime)
{
    m_animator->update(deltaTime);
}

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

    m_model->render(
        renderer, shader, camera, model, light, m_animator->skinMatrices());
}

void Hand::debugAdjust(const InputManager& input, const float deltaTime)
{
    const float scaleSpeed = 0.5F * deltaTime;
    const float rotationSpeed = 0.5F * deltaTime;
    const float moveSpeed = 0.05F * deltaTime;

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
