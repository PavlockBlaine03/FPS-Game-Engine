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

    // Start well inside the positive-Z room instead of in the dividing
    // doorway. The X offset also keeps the player clear of the physics stack.
    constexpr glm::vec3 PLAYER_START_POSITION(5.0F, 0.45F, 6.0F);

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
    , m_camera(std::make_unique<Camera>(
        PLAYER_START_POSITION + glm::vec3(0.0F, EYE_HEIGHT_OFFSET, 0.0F)))
    , m_renderer(std::make_unique<Renderer>(width, height))
    , m_textRenderer(std::make_unique<TextRenderer>(FONT_PATH, FONT_SIZE))
    , m_scene(std::make_unique<Scene>())
    , m_pistol(std::make_unique<Pistol>())
    , m_hand(std::make_unique<Hand>())
    , m_physics(std::make_unique<PhysicsWorld>())
    , m_rigidBodies(std::make_unique<RigidBodyWorld>())
    , m_projectiles(std::make_unique<ProjectileManager>())
    , m_particles(std::make_unique<ParticleSystem>())
    , m_bodyPosition(PLAYER_START_POSITION)
    , m_width(width)
    , m_height(height)
{
    const float doorHeight = Scene::DOORWAY_HEIGHT - Scene::FLOOR_TOP_Y;
    m_doors.push_back(std::make_unique<Door>(
        glm::vec3(-Scene::DOORWAY_HALF_WIDTH, Scene::FLOOR_TOP_Y, -DOOR_THICKNESS * 0.5F),
        Scene::DOORWAY_HALF_WIDTH * 2.0F, doorHeight,
        DOOR_THICKNESS, DOOR_TEXTURE_PATH));

    // Side-wall openings run along Z, so their doors are the same mesh rotated
    // 90 degrees. North doors hinge at the high-Z edge; south doors at low Z.
    for (const float wallX : { -4.0F, 4.0F })
    {
        m_doors.push_back(std::make_unique<Door>(
            glm::vec3(wallX, Scene::FLOOR_TOP_Y, 6.5F),
            1.0F, doorHeight, DOOR_THICKNESS, DOOR_TEXTURE_PATH, 90.0F));
        m_doors.push_back(std::make_unique<Door>(
            glm::vec3(wallX, Scene::FLOOR_TOP_Y, -6.5F),
            1.0F, doorHeight, DOOR_THICKNESS, DOOR_TEXTURE_PATH, -90.0F));
    }

    m_physics->setColliders(m_scene->colliders());
    m_rigidBodies->spawnCubeStack();
    m_rigidBodies->spawnBlueBall();
    m_rigidBodies->spawnCubeStack(glm::vec3(-8.0F, Scene::FLOOR_TOP_Y, 8.0F), 3);
    m_rigidBodies->spawnBlueBall(glm::vec3(-10.0F, 0.8F, 3.0F));
    m_rigidBodies->spawnCubeStack(glm::vec3(8.0F, Scene::FLOOR_TOP_Y, 9.0F), 3);
    m_rigidBodies->spawnBlueBall(glm::vec3(10.0F, 0.8F, 3.0F));
    m_rigidBodies->spawnCubeStack(glm::vec3(-8.0F, Scene::FLOOR_TOP_Y, -8.0F), 3);
    m_rigidBodies->spawnBlueBall(glm::vec3(-10.0F, 0.8F, -3.0F));
    m_rigidBodies->spawnCubeStack(glm::vec3(8.0F, Scene::FLOOR_TOP_Y, -8.0F), 3);
    m_rigidBodies->spawnBlueBall(glm::vec3(10.0F, 0.8F, -3.0F));

    // A central ceiling light with stronger ambient fill keeps all six rooms
    // readable until the renderer supports multiple point lights.
    m_light.position = glm::vec3(0.0F, 2.3F, 0.0F);
    m_light.color = glm::vec3(2.2F, 2.1F, 1.9F);
    m_light.ambientStrength = 0.28F;
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
            for (int i = 0; i < 3; i++) {
                m_persons.push_back(std::make_unique<Person>(
                    m_personModel,
                    glm::vec3(-2.0F + (i * 2), Scene::FLOOR_TOP_Y, -2.0F),
                    glm::vec3(-2.0F + (i * 2), Scene::FLOOR_TOP_Y, -6.0F),
                    (i * 0.5F) + 0.5F));
            }
            for (const float roomX : { -8.0F, 8.0F })
            {
                m_persons.push_back(std::make_unique<Person>(
                    m_personModel,
                    glm::vec3(roomX, Scene::FLOOR_TOP_Y, 3.0F),
                    glm::vec3(roomX, Scene::FLOOR_TOP_Y, 10.0F), 0.8F));
                m_persons.push_back(std::make_unique<Person>(
                    m_personModel,
                    glm::vec3(roomX, Scene::FLOOR_TOP_Y, -3.0F),
                    glm::vec3(roomX, Scene::FLOOR_TOP_Y, -10.0F), 0.8F));
            }
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

    m_worldBuilder = std::make_unique<WorldBuilder>(m_personModel);
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

    if (m_input->isKeyJustPressed(GLFW_KEY_F1))
    {
        m_worldBuilderActive = !m_worldBuilderActive;
        if (m_worldBuilderActive)
            m_camera->setPosition(glm::vec3(8.0F, 6.0F, 8.0F));
        else
        {
            m_worldBuilder->syncDynamicObjects(*m_rigidBodies);
            m_camera->setPosition(m_bodyPosition + glm::vec3(0.0F, EYE_HEIGHT_OFFSET, 0.0F));
        }
        m_input->resetMouseDelta();
        return;
    }

    if (m_input->isKeyPressed(GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(m_window->handle(), GLFW_TRUE);
    }

    if (m_worldBuilderActive)
    {
        m_worldBuilder->update(*m_input, *m_camera, m_time.deltaTime());
        m_input->resetMouseDelta();
        return;
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
    const bool customWorld = m_worldBuilder->hasPieces();
    std::vector<AABB> colliders = customWorld
        ? m_worldBuilder->colliders()
        : m_scene->colliders();
    std::vector<std::unique_ptr<Door>>& activeDoors = customWorld
        ? m_worldBuilder->doors()
        : m_doors;

    for (const std::unique_ptr<Door>& door : activeDoors)
        if (door->isBlocking()) colliders.push_back(door->collider());
    const std::vector<AABB> projectileColliders = colliders;
    std::vector<std::unique_ptr<Person>>& activePersons = customWorld
        ? m_worldBuilder->dummies()
        : m_persons;
    std::vector<AABB> npcColliders;
    npcColliders.reserve(activePersons.size());
    for (const std::unique_ptr<Person>& person : activePersons) {
        if (person != nullptr)
        {
            person->update(m_time.deltaTime(), *m_rigidBodies);
            if (!person->isRagdoll())
            {
                const AABB personCollider = person->collider();
                colliders.push_back(personCollider);
                npcColliders.push_back(personCollider);
            }
            else
            {
                // Keep one stable PhysX collider slot per Person. An invalid
                // AABB tells RigidBodyWorld to disable this kinematic body.
                npcColliders.push_back(AABB{ glm::vec3(1.0F), glm::vec3(-1.0F) });
            }
        }
    }
    // Keep a dynamic-body-free copy for projectile world tests. Rigid bodies
    // are raycast directly below, so they cannot be mistaken for static walls.
    std::vector<AABB> aimColliders = colliders;
    const std::vector<AABB> rigidColliders = m_rigidBodies->colliders();
    aimColliders.insert(aimColliders.end(), rigidColliders.begin(), rigidColliders.end());

    m_physics->setColliders(colliders);

    // Physics simulates the body's center position; the camera is derived
    // from it with a fixed vertical eye-height offset, so raising/lowering
    // the camera doesn't require changing the collider size at all.
    m_physics->update(m_bodyPosition, moveInput, m_time.deltaTime());
    m_camera->setPosition(m_bodyPosition + glm::vec3(0.0F, EYE_HEIGHT_OFFSET, 0.0F));

    m_camera->processMouseMovement(m_input->mouseDeltaX(), m_input->mouseDeltaY());
    m_input->resetMouseDelta();
    m_pistol->update(m_time.deltaTime());
    m_hand->update(m_time.deltaTime());

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
            aimColliders);
        m_particles->spawnBurst(muzzlePosition, fireDirection);
        m_audio->play(AudioEngine::AudioType::PISTOL_SHOT);
        m_pistol->triggerRecoil();
    }

    // Interact with the door on a fresh 'E' press, only if the player is
    // within range of it.
    if (m_input->isInteractKeyJustPressed())
    {
        Door* nearestDoor = nullptr;
        float nearestDistance = DOOR_INTERACT_RANGE;
        for (const std::unique_ptr<Door>& door : activeDoors)
        {
            const AABB doorBounds = door->collider();
            const glm::vec3 doorCenter = (doorBounds.min + doorBounds.max) * 0.5F;
            const float distance = glm::distance(m_bodyPosition, doorCenter);
            if (distance <= nearestDistance)
            {
                nearestDistance = distance;
                nearestDoor = door.get();
            }
        }
        if (nearestDoor != nullptr) 
        {
            nearestDoor->interact();
            if (nearestDoor->getState() == State::Opening) 
            {
                m_audio->play(AudioEngine::AudioType::DOOR_OPENING);
            }
            if (nearestDoor->getState() == State::Closing)
            {
                m_audio->play(AudioEngine::AudioType::DOOR_CLOSING);
            }
        }
    }

    for (const std::unique_ptr<Door>& door : activeDoors)
        door->update(m_time.deltaTime());

    // Dynamic bodies use static room/door geometry, but not their own AABBs.
    std::vector<AABB> rigidStatics = customWorld
        ? m_worldBuilder->colliders()
        : m_scene->colliders();
    for (const std::unique_ptr<Door>& door : activeDoors)
        if (door->isBlocking()) rigidStatics.push_back(door->collider());
    m_rigidBodies->movePlayerCollider(
        m_bodyPosition, glm::vec3(0.3F, 0.9F, 0.3F), m_time.deltaTime());
    m_rigidBodies->moveNpcColliders(npcColliders, m_time.deltaTime());
    m_rigidBodies->update(m_time.deltaTime(), rigidStatics);
    m_projectiles->update(m_time.deltaTime(), projectileColliders, activePersons,
        *m_particles, *m_rigidBodies);
    m_particles->update(m_time.deltaTime());
}

void Application::render() const
{
    m_renderer->beginFrame();

    if (m_worldBuilderActive)
    {
        m_worldBuilder->renderWorld(*m_renderer, *m_shader, *m_skeletalShader,
            *m_camera, m_light);
        m_worldBuilder->renderUi(*m_textRenderer, *m_textShader, m_width, m_height);
        m_textRenderer->renderCrosshair(*m_textShader, m_width, m_height,
            0.5F, glm::vec3(1.0F, 0.75F, 0.15F));
        return;
    }

    const bool customWorld = m_worldBuilder->hasPieces();
    if (customWorld)
        m_worldBuilder->renderPieces(*m_renderer, *m_shader, *m_skeletalShader,
            *m_camera, m_light, false);
    else
        m_scene->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_rigidBodies->render(*m_renderer, *m_shader, *m_camera, m_light);
    if (!customWorld)
        for (const std::unique_ptr<Door>& door : m_doors)
            door->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_projectiles->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_particles->render(*m_renderer, *m_shader, *m_camera, m_light);
    const std::vector<std::unique_ptr<Person>>& renderedPersons = customWorld
        ? m_worldBuilder->dummies()
        : m_persons;
    if (!customWorld) for (const std::unique_ptr<Person>& person : renderedPersons) {
        if (person != nullptr)
        {
            person->render(*m_renderer, *m_skeletalShader, *m_camera, m_light);
        }
    }

    // Clear depth so the viewmodel always renders on top of world geometry,
    // preventing it from clipping into walls/floor when the camera gets close.
    glClear(GL_DEPTH_BUFFER_BIT);
    m_pistol->render(*m_renderer, *m_shader, *m_camera, m_light);
    m_hand->render(*m_renderer, *m_skeletalShader, *m_camera, m_light, *m_pistol);

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
