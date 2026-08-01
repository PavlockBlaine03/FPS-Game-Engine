#include "core/Application.h"
#include "rendering/SkeletalModel.h"
#include "util/MeshFactory.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <format>
#include <filesystem>
#include <iostream>

namespace
{
    constexpr const char* FONT_PATH = "assets/fonts/ARIAL.TTF";
    constexpr unsigned int FONT_SIZE = 48;

    constexpr const char* BASIC_VERTEX_SHADER_PATH = "assets/shaders/basic.vert";
    constexpr const char* BASIC_FRAGMENT_SHADER_PATH = "assets/shaders/basic.frag";

    constexpr const char* TEXT_VERTEX_SHADER_PATH = "assets/shaders/text.vert";
    constexpr const char* TEXT_FRAGMENT_SHADER_PATH = "assets/shaders/text.frag";
    constexpr const char* SKINNED_VERTEX_SHADER_PATH = "assets/shaders/skinned.vert";
    constexpr const char* PERSON_MODEL_PATH = "assets/models/Characters/Person/person.glb";

    constexpr const char* DOOR_TEXTURE_PATH = "assets/textures/wooddoor.png";
    constexpr float DOOR_THICKNESS = 0.075f;

    // Vertical offset from the physics body's center to the camera's eye
    // position. With the player collider's half-height at 0.9 (1.8 total),
    // an offset of ~0.7 puts the eyes near the top of the collider rather
    // than dead-center (stomach height).
    constexpr float EYE_HEIGHT_OFFSET = 0.7F;

    // Maximum distance from the door's center the player can be to
    // interact with it.
    constexpr float DOOR_INTERACT_RANGE = 2.0F;
}

Application::Application(const int width, const int height, const std::string& title)
    : m_window(std::make_unique<Window>(width, height, title))
    , m_input(std::make_unique<InputManager>(m_window->handle()))
    , m_audio(std::make_unique<AudioEngine>())
    , m_shader(std::make_unique<Shader>(
        Shader::fromFiles(BASIC_VERTEX_SHADER_PATH, BASIC_FRAGMENT_SHADER_PATH)))
    , m_textShader(std::make_unique<Shader>(
        Shader::fromFiles(TEXT_VERTEX_SHADER_PATH, TEXT_FRAGMENT_SHADER_PATH)))
    , m_skeletalShader(std::make_unique<Shader>(
        Shader::fromFiles(SKINNED_VERTEX_SHADER_PATH, BASIC_FRAGMENT_SHADER_PATH)))
    , m_camera(std::make_unique<Camera>(glm::vec3(0.0F, 1.0F + EYE_HEIGHT_OFFSET, 0.0F)))
    , m_renderer(std::make_unique<Renderer>(width, height))
    , m_textRenderer(std::make_unique<TextRenderer>(FONT_PATH, FONT_SIZE))
    , m_scene(std::make_unique<Scene>())
    , m_pistol(std::make_unique<Pistol>())
    , m_hand(std::make_unique<Hand>())
    , m_physics(std::make_unique<PhysicsWorld>())
    , m_projectiles(std::make_unique<ProjectileManager>())
    , m_particles(std::make_unique<ParticleSystem>())
    , m_door(std::make_unique<Door>(
        // Hinge at the doorway's left edge (matching Scene's doorway gap),
        // spanning its clear width/height, matching wall thickness.
        glm::vec3(-Scene::DOORWAY_HALF_WIDTH, Scene::FLOOR_TOP_Y, -DOOR_THICKNESS * 0.5F),
        Scene::DOORWAY_HALF_WIDTH * 2.0F,
        Scene::DOORWAY_HEIGHT - Scene::FLOOR_TOP_Y,
        DOOR_THICKNESS,
        DOOR_TEXTURE_PATH))
    , m_bodyPosition(0.0F, 1.0F, 0.0F)
    , m_width(width)
    , m_height(height)
{
    m_physics->setColliders(m_scene->colliders());

    // Point light hanging from the ceiling, roughly centered in the room.
    // (z in [-8, 0], so center is roughly z = -4).
    m_light.position = glm::vec3(0.0F, 2.3F, -4.0F);
    m_light.color = glm::vec3(2.2F, 2.1F, 1.9F);
    m_light.ambientStrength = 0.15F;
    m_light.specularStrength = 0.4F;
    m_light.shininess = 32.0F;

    const glm::mat4 textProjection = glm::ortho(
        0.0F,
        static_cast<float>(m_width),
        0.0F,
        static_cast<float>(m_height)
    );

    m_textShader->use();
    m_textShader->setMat4("projection", textProjection);

    if (std::filesystem::exists(PERSON_MODEL_PATH))
    {
        try
        {
            m_personModel = std::make_shared<SkeletalModel>(PERSON_MODEL_PATH);
            m_person = std::make_unique<Person>(
                m_personModel,
                glm::vec3(-2.0F, Scene::FLOOR_TOP_Y, -2.0F),
                glm::vec3(-2.0F, Scene::FLOOR_TOP_Y, -6.0F));
        }
        catch (const std::exception& error)
        {
            std::cerr << "Animated person disabled: " << error.what() << '\n';
        }
    }
    else
    {
        std::cerr << "Animated person disabled: add a humanoid with Idle and Walk clips at "
                  << PERSON_MODEL_PATH << '\n';
    }
}

Application::~Application() = default;

void Application::run()
{
    while (!m_window->shouldClose())
    {
        m_time.update();

        processInput();
        render();

        m_window->swapBuffers();
        Window::pollEvents();
    }
}

void Application::processInput()
{
    m_input->update();

    if (m_input->isKeyPressed(GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(m_window->handle(), GLFW_TRUE);
    }

    // Build a horizontal move direction from the camera's forward/right
    // vectors, flattened onto the XZ plane so looking up/down doesn't
    // affect walking speed.
    glm::vec3 forward = m_camera->front();
    forward.y = 0.0F;

    glm::vec3 right = m_camera->right();
    right.y = 0.0F;

    if (glm::length(forward) > 0.0001F) { forward = glm::normalize(forward); }
    if (glm::length(right) > 0.0001F) { right = glm::normalize(right); }

    glm::vec3 moveInput(0.0F);

    if (m_input->isKeyPressed(GLFW_KEY_W)) { moveInput += forward; }
    if (m_input->isKeyPressed(GLFW_KEY_S)) { moveInput -= forward; }
    if (m_input->isKeyPressed(GLFW_KEY_D)) { moveInput += right; }
    if (m_input->isKeyPressed(GLFW_KEY_A)) { moveInput -= right; }

    if (glm::length(moveInput) > 0.0001F)
    {
        moveInput = glm::normalize(moveInput);
    }

    if (m_input->isKeyPressed(GLFW_KEY_SPACE))
    {
        m_physics->jump();
    }

    // Rebuild the collider list each frame: the static scene geometry plus
    // the door's collider only while it's fully closed. This is simpler
    // than trying to track a rotated collider mid-swing, and cheap enough
    // given the scene's collider count.
    std::vector<AABB> colliders = m_scene->colliders();

    if (m_door->isBlocking())
    {
        colliders.push_back(m_door->collider());
    }

    if (m_person != nullptr)
    {
        m_person->update(m_time.deltaTime());
        colliders.push_back(m_person->collider());
    }

    m_physics->setColliders(colliders);

    // Physics simulates the body's center position; the camera is derived
    // from it with a fixed vertical eye-height offset, so raising/lowering
    // the camera doesn't require changing the collider size at all.
    m_physics->update(m_bodyPosition, moveInput, m_time.deltaTime());
    m_camera->setPosition(m_bodyPosition + glm::vec3(0.0F, EYE_HEIGHT_OFFSET, 0.0F));

    m_camera->processMouseMovement(m_input->mouseDeltaX(), m_input->mouseDeltaY());
    m_input->resetMouseDelta();
    m_pistol->debugAdjust(*m_input, m_time.deltaTime());
    m_pistol->update(m_time.deltaTime());
    m_hand->debugAdjust(*m_input, m_time.deltaTime());

    // Fire on a fresh left-click (not held) -- spawns both a projectile and
    // a muzzle-flash particle burst at the pistol's muzzle.
    if (m_input->isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        const glm::vec3 muzzlePosition = m_pistol->muzzleWorldPosition(*m_camera);
        const glm::vec3 fireDirection = m_pistol->muzzleWorldDirection(*m_camera);

        m_projectiles->spawnAimed(
            muzzlePosition,
            m_camera->position(),
            fireDirection,
            colliders);
        m_particles->spawnBurst(muzzlePosition, fireDirection);
        m_audio->play("assets/audio/weapons/pistol-shot.wav");
        m_pistol->triggerRecoil();
    }

    // Interact with the door on a fresh 'E' press, only if the player is
    // within range of it.
    if (m_input->isInteractKeyJustPressed())
    {
        const glm::vec3 doorCenter = m_door->hingePosition() +
            glm::vec3(Scene::DOORWAY_HALF_WIDTH, Scene::DOORWAY_HEIGHT * 0.5F, 0.0F);

        if (glm::distance(m_bodyPosition, doorCenter) <= DOOR_INTERACT_RANGE)
        {
            m_door->interact();
        }
    }

    m_door->update(m_time.deltaTime());

    m_projectiles->update(m_time.deltaTime(), colliders, *m_particles);
    m_particles->update(m_time.deltaTime());
}

void Application::render() const
{
    m_renderer->beginFrame();

    m_scene->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_door->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_projectiles->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_particles->render(*m_renderer, *m_shader, *m_camera, m_light);
    if (m_person != nullptr)
    {
        m_person->render(*m_renderer, *m_skeletalShader, *m_camera, m_light);
    }

    // Clear depth so the viewmodel always renders on top of world geometry,
    // preventing it from clipping into walls/floor when the camera gets close.
    glClear(GL_DEPTH_BUFFER_BIT);
    m_pistol->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_hand->render(*m_renderer, *m_shader, *m_camera, m_light, *m_pistol);

    const glm::vec3& pos = m_camera->position();

    const std::string coordinateText = std::format(
        "X: {:.2f}  Y: {:.2f}  Z: {:.2f}",
        pos.x,
        pos.y,
        pos.z
    );

    m_textRenderer->renderText(
        *m_textShader,
        coordinateText,
        10.0F,
        static_cast<float>(m_height) - 80.0F,
        1.0F,
        glm::vec3(1.0F, 1.0F, 1.0F)
    );

    m_textRenderer->renderCrosshair(
        *m_textShader,
        m_width,
        m_height,
        0.5F,
        glm::vec3(1.0F, 1.0F, 1.0F)
    );
}
