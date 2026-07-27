#include "scene/Entity/Weapons/Pistol.h"
#include "rendering/Renderer.h"
#include "rendering/Camera.h"
#include "input/InputManager.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace
{
    constexpr const char* PISTOL_MODEL_PATH = "assets/models/Weapons/p_38_Pistol.fbx";
}

Pistol::Pistol()
    : Weapon(2.0F) // fires ~2 rounds/sec, placeholder for later
    , m_model(std::make_unique<Model>(PISTOL_MODEL_PATH))
    , m_viewOffset(0.3F, -0.25F, -0.75F) // bottom-right of screen, in front of camera
    , m_viewRotationDegrees(0.0F, 0.0F, 0.0F) // most exported models face +z; flip to face away from camera
    , m_viewScale(0.003F) // start much smaller than before; tune from here
    , m_muzzleLocalOffset(0.0F, 0.1F, -0.3F)
{
}

Pistol::~Pistol() = default;

void Pistol::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light) const
{
    const glm::mat4 cameraLocal = glm::inverse(camera.viewMatrix());

    glm::mat4 model = cameraLocal;
    model = glm::translate(model, m_viewOffset);
    model = glm::rotate(model, glm::radians(m_viewRotationDegrees.y), glm::vec3(0.0F, 1.0F, 0.0F));
    model = glm::rotate(model, glm::radians(m_viewRotationDegrees.x), glm::vec3(1.0F, 0.0F, 0.0F));
    model = glm::scale(model, glm::vec3(m_viewScale));

    m_model->render(renderer, shader, camera, model, light);
}
glm::vec3 Pistol::muzzleWorldPosition(const Camera& camera) const
{
    const glm::mat4 cameraLocal = glm::inverse(camera.viewMatrix());

    // Same transform chain as render(), but without the final mesh scale --
    // the muzzle offset is already expressed in view-relative world units,
    // not the model's own (tiny) local scale.
    glm::mat4 transform = cameraLocal;
    transform = glm::translate(transform, m_viewOffset);
    transform = glm::rotate(transform, glm::radians(m_viewRotationDegrees.y), glm::vec3(0.0F, 1.0F, 0.0F));
    transform = glm::rotate(transform, glm::radians(m_viewRotationDegrees.x), glm::vec3(1.0F, 0.0F, 0.0F));

    const glm::vec4 muzzleWorld = transform * glm::vec4(m_muzzleLocalOffset, 1.0F);

    return glm::vec3(muzzleWorld);
}

glm::vec3 Pistol::muzzleWorldDirection(const Camera& camera) const
{
    // Bullets fire straight down the camera's look direction; the muzzle
    // position is just where they visually originate from.
    return camera.front();
}
void Pistol::debugAdjust(const InputManager& input, const float deltaTime)
{
    const float scaleSpeed = 0.5F * deltaTime;
    const float rotationSpeed = 90.0F * deltaTime;
    const float moveSpeed = 2.0f * deltaTime;

    if (input.isKeyPressed(GLFW_KEY_KP_ADD)) { m_viewScale *= (1.0F + scaleSpeed); }
    if (input.isKeyPressed(GLFW_KEY_KP_SUBTRACT)) { m_viewScale *= (1.0F - scaleSpeed); }

    if (input.isKeyPressed(GLFW_KEY_KP_4)) { m_viewRotationDegrees.y += rotationSpeed; }
    if (input.isKeyPressed(GLFW_KEY_KP_6)) { m_viewRotationDegrees.y -= rotationSpeed; }
    if (input.isKeyPressed(GLFW_KEY_KP_8)) { m_viewRotationDegrees.x += rotationSpeed; }
    if (input.isKeyPressed(GLFW_KEY_KP_2)) { m_viewRotationDegrees.x -= rotationSpeed; }

    if (input.isKeyPressed(GLFW_KEY_UP))    { m_viewOffset.y += moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_DOWN))  { m_viewOffset.y -= moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_LEFT))  { m_viewOffset.x -= moveSpeed; }
    if (input.isKeyPressed(GLFW_KEY_RIGHT)) { m_viewOffset.x += moveSpeed; }

    if (input.isKeyPressed(GLFW_KEY_KP_5))
    {
        std::cout << "Pistol scale=" << m_viewScale
                   << " rotation=(" << m_viewRotationDegrees.x << ", "
                   << m_viewRotationDegrees.y << ")\n" 
                   << "Position= " << m_viewOffset.x << ", " << m_viewOffset.y << "\n";
    }
}