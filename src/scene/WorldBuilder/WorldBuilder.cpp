#include "scene/WorldBuilder/WorldBuilder.h"

#include <glad/glad.h>

#include "input/InputManager.h"
#include "rendering/Camera.h"
#include "rendering/Mesh.h"
#include "rendering/Model.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/SkeletalModel.h"
#include "rendering/TextRenderer.h"
#include "rendering/Texture.h"
#include "physics/RigidBodyWorld.h"
#include "scene/Entity/Characters/Person.h"
#include "scene/Entity/Objects/Door.h"
#include "util/MeshFactory.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace
{
    constexpr const char* STAIR_MODEL_PATH = "assets/models/Environment/Stairs/scala.fbx";
    constexpr float SNAP_SIZE = 0.25F;
    constexpr BuildPieceType PIECES[] = {
        BuildPieceType::Floor, BuildPieceType::Wall,
        BuildPieceType::Stairs, BuildPieceType::Door,
        BuildPieceType::Cube, BuildPieceType::Sphere, BuildPieceType::Dummy
    };
    constexpr int PIECE_COUNT = 7;
    constexpr const char* FLOOR_TEXTURES[] = {
        "assets/textures/flooring.png", "assets/textures/ceiling.png"
    };
    constexpr const char* WALL_TEXTURES[] = {
        "assets/textures/wallpaper1.png", "assets/textures/ceiling.png"
    };
    constexpr const char* DOOR_TEXTURE = "assets/textures/wooddoor.png";

    glm::mat4 cubeTransform(const glm::vec3& position, const glm::vec3& scale)
    {
        glm::mat4 result = glm::translate(glm::mat4(1.0F), position);
        return glm::scale(result, scale);
    }

    std::filesystem::path savePath()
    {
        return std::filesystem::path(FPSGAME_SOURCE_DIR) / "worlds" / "editor_world.world";
    }

    AABB rotatedBox(const glm::vec3& anchor, const glm::vec3& localCenter,
        const glm::vec3& halfExtents, const float yawDegrees)
    {
        const float radians = glm::radians(yawDegrees);
        const float sine = std::sin(radians);
        const float cosine = std::cos(radians);
        const glm::vec3 center = anchor + glm::vec3(
            localCenter.x * cosine + localCenter.z * sine,
            localCenter.y,
            -localCenter.x * sine + localCenter.z * cosine);
        const glm::vec3 rotatedHalf(
            std::abs(cosine) * halfExtents.x + std::abs(sine) * halfExtents.z,
            halfExtents.y,
            std::abs(sine) * halfExtents.x + std::abs(cosine) * halfExtents.z);
        return AABB::fromCenterHalfExtents(center, rotatedHalf);
    }
}

WorldBuilder::WorldBuilder(std::shared_ptr<SkeletalModel> personModel)
    : m_personModel(std::move(personModel))
    , m_doorTexture(std::make_unique<Texture>(DOOR_TEXTURE))
    , m_cubeTexture(std::make_unique<Texture>(glm::vec3(0.72F, 0.22F, 0.12F)))
    , m_sphereTexture(std::make_unique<Texture>(glm::vec3(0.08F, 0.25F, 0.9F)))
    , m_gridTexture(std::make_unique<Texture>(glm::vec3(0.18F, 0.20F, 0.23F)))
    , m_xAxisTexture(std::make_unique<Texture>(glm::vec3(0.9F, 0.12F, 0.12F)))
    , m_yAxisTexture(std::make_unique<Texture>(glm::vec3(0.15F, 0.85F, 0.2F)))
    , m_zAxisTexture(std::make_unique<Texture>(glm::vec3(0.15F, 0.35F, 0.95F)))
    , m_previewTexture(std::make_unique<Texture>(glm::vec3(0.95F, 0.72F, 0.12F)))
{
    for (const char* path : FLOOR_TEXTURES)
        m_floorTextures.push_back(std::make_unique<Texture>(path));
    for (const char* path : WALL_TEXTURES)
        m_wallTextures.push_back(std::make_unique<Texture>(path));
    const MeshData cube = MeshFactory::createCube(1.0F, 1.0F);
    m_cubeMesh = std::make_unique<Mesh>(cube.vertices.data(), cube.vertices.size(),
        cube.indices.data(), cube.indices.size(), 8);
    const MeshData sphere = MeshFactory::createSphere();
    m_sphereMesh = std::make_unique<Mesh>(sphere.vertices.data(), sphere.vertices.size(),
        sphere.indices.data(), sphere.indices.size(), 8);
    if (std::filesystem::exists(STAIR_MODEL_PATH))
        m_stairModel = std::make_unique<Model>(STAIR_MODEL_PATH);
}

WorldBuilder::~WorldBuilder() = default;

void WorldBuilder::update(const InputManager& input, Camera& camera, const float deltaTime)
{
    if (input.isKeyJustPressed(GLFW_KEY_TAB)) m_menuOpen = !m_menuOpen;
    if (input.isKeyJustPressed(GLFW_KEY_G))
    {
        m_snapEnabled = !m_snapEnabled;
        m_status = m_snapEnabled ? "Grid snap enabled" : "Free placement enabled";
    }

    if (m_menuOpen)
    {
        if (input.isKeyJustPressed(GLFW_KEY_UP))
            m_selectedIndex = (m_selectedIndex + PIECE_COUNT - 1) % PIECE_COUNT;
        if (input.isKeyJustPressed(GLFW_KEY_DOWN))
            m_selectedIndex = (m_selectedIndex + 1) % PIECE_COUNT;
        if (input.isKeyJustPressed(GLFW_KEY_ENTER))
        {
            m_menuOpen = false;
            m_status = std::string("Selected ") + pieceName(PIECES[m_selectedIndex]);
        }
    }

    glm::vec3 movement(0.0F);
    if (input.isKeyPressed(GLFW_KEY_W)) movement.z += 1.0F;
    if (input.isKeyPressed(GLFW_KEY_S)) movement.z -= 1.0F;
    if (input.isKeyPressed(GLFW_KEY_D)) movement.x += 1.0F;
    if (input.isKeyPressed(GLFW_KEY_A)) movement.x -= 1.0F;
    if (input.isKeyPressed(GLFW_KEY_E)) movement.y += 1.0F;
    if (input.isKeyPressed(GLFW_KEY_Q)) movement.y -= 1.0F;
    if (glm::length(movement) > 0.001F) movement = glm::normalize(movement);
    camera.processKeyboard(movement, deltaTime * 2.0F);
    camera.processMouseMovement(input.mouseDeltaX(), input.mouseDeltaY());

    if (input.isKeyJustPressed(GLFW_KEY_PAGE_UP)) m_placementHeight += SNAP_SIZE;
    if (input.isKeyJustPressed(GLFW_KEY_PAGE_DOWN)) m_placementHeight -= SNAP_SIZE;
    if (input.isKeyJustPressed(GLFW_KEY_R))
        m_yawDegrees += m_snapEnabled ? 90.0F : 15.0F;
    if (PIECES[m_selectedIndex] == BuildPieceType::Wall
        && input.isKeyJustPressed(GLFW_KEY_C))
    {
        m_wallVariant = static_cast<WallVariant>((static_cast<int>(m_wallVariant) + 1) % 3);
        m_status = std::string("Wall type: ") + wallVariantName();
    }
    if (input.isKeyJustPressed(GLFW_KEY_T))
    {
        if (PIECES[m_selectedIndex] == BuildPieceType::Floor)
            m_floorMaterialIndex = (m_floorMaterialIndex + 1)
                % static_cast<int>(m_floorTextures.size());
        else if (PIECES[m_selectedIndex] == BuildPieceType::Wall)
            m_wallMaterialIndex = (m_wallMaterialIndex + 1)
                % static_cast<int>(m_wallTextures.size());
        m_status = std::string("Material: ") + materialName();
    }
    if (PIECES[m_selectedIndex] == BuildPieceType::Cube
        || PIECES[m_selectedIndex] == BuildPieceType::Sphere)
    {
        const float scaleStep = m_snapEnabled ? 0.25F : 0.1F;
        if (input.isKeyJustPressed(GLFW_KEY_LEFT_BRACKET))
            m_objectScale = std::max(0.1F, m_objectScale - scaleStep);
        if (input.isKeyJustPressed(GLFW_KEY_RIGHT_BRACKET))
            m_objectScale = std::min(10.0F, m_objectScale + scaleStep);
    }

    const glm::vec3 point = camera.position() + camera.front() * 5.0F;
    m_previewPosition = glm::vec3(point.x, m_placementHeight, point.z);
    if (m_snapEnabled)
    {
        m_previewPosition.x = std::round(m_previewPosition.x / SNAP_SIZE) * SNAP_SIZE;
        m_previewPosition.y = std::round(m_previewPosition.y / SNAP_SIZE) * SNAP_SIZE;
        m_previewPosition.z = std::round(m_previewPosition.z / SNAP_SIZE) * SNAP_SIZE;
    }

    if (!m_menuOpen && input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) placePiece();
    if (!m_menuOpen && input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) removePiece();

    const bool control = input.isKeyPressed(GLFW_KEY_LEFT_CONTROL)
        || input.isKeyPressed(GLFW_KEY_RIGHT_CONTROL);
    if (control && input.isKeyJustPressed(GLFW_KEY_S)) save();
    if (control && input.isKeyJustPressed(GLFW_KEY_L)) load();
}

void WorldBuilder::placePiece()
{
    const BuildPieceType type = PIECES[m_selectedIndex];
    const int material = type == BuildPieceType::Floor
        ? m_floorMaterialIndex : type == BuildPieceType::Wall ? m_wallMaterialIndex : 0;
    m_pieces.push_back(BuildPiece{
        type, m_previewPosition, m_yawDegrees, m_wallVariant, material, m_objectScale });
    rebuildDummies();
    rebuildDoors();
    rebuildColliders();
    m_status = std::string("Placed ") + pieceName(PIECES[m_selectedIndex]);
}

void WorldBuilder::removePiece()
{
    if (m_pieces.empty()) return;
    auto closest = m_pieces.end();
    float distance = 1.5F;
    for (auto piece = m_pieces.begin(); piece != m_pieces.end(); ++piece)
    {
        const float candidate = glm::distance(piece->position, m_previewPosition);
        if (candidate < distance) { distance = candidate; closest = piece; }
    }
    if (closest != m_pieces.end())
    {
        m_pieces.erase(closest);
        rebuildDummies();
        rebuildDoors();
        rebuildColliders();
        m_status = "Removed piece";
    }
}

glm::mat4 WorldBuilder::pieceTransform(const BuildPiece& piece) const
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0F), piece.position);
    transform = glm::rotate(transform, glm::radians(piece.yawDegrees), glm::vec3(0.0F, 1.0F, 0.0F));
    if (piece.type == BuildPieceType::Floor)
    {
        transform = glm::translate(transform, glm::vec3(0.0F, 0.05F, 0.0F));
        return glm::scale(transform, glm::vec3(2.0F, 0.1F, 2.0F));
    }
    if (piece.type == BuildPieceType::Wall)
    {
        transform = glm::translate(transform, glm::vec3(0.0F, 1.25F, 0.0F));
        return glm::scale(transform, glm::vec3(2.0F, 2.5F, 0.1F));
    }
    if (piece.type == BuildPieceType::Stairs && m_stairModel != nullptr)
    {
        const AABB bounds = m_stairModel->localBounds();
        const glm::vec3 size = bounds.max - bounds.min;
        const glm::vec3 center = (bounds.min + bounds.max) * 0.5F;
        const bool longX = size.x > size.z;
        transform = glm::translate(transform, glm::vec3(0.0F, 1.825F, 0.0F));
        if (longX) transform = glm::rotate(transform, glm::radians(90.0F), glm::vec3(0, 1, 0));
        transform = glm::scale(transform, longX
            ? glm::vec3(4.0F / size.x, 3.65F / size.y, 1.6F / size.z)
            : glm::vec3(1.6F / size.x, 3.65F / size.y, 4.0F / size.z));
        return glm::translate(transform, -center);
    }
    if (piece.type == BuildPieceType::Door)
    {
        transform = glm::translate(transform, glm::vec3(0.0F, 1.0F, 0.0F));
        return glm::scale(transform, glm::vec3(1.0F, 2.0F, 0.075F));
    }
    if (piece.type == BuildPieceType::Cube || piece.type == BuildPieceType::Sphere)
    {
        transform = glm::translate(transform, glm::vec3(0.0F, piece.scale * 0.5F, 0.0F));
        return glm::scale(transform, glm::vec3(piece.scale));
    }
    transform = glm::translate(transform, glm::vec3(0.0F, 0.9F, 0.0F));
    return glm::scale(transform, glm::vec3(0.6F, 1.8F, 0.6F));
}

void WorldBuilder::renderWorld(const Renderer& renderer, const Shader& shader,
    const Shader& skeletalShader, const Camera& camera, const Light& light) const
{
    for (int line = -20; line <= 20; ++line)
    {
        renderer.draw(*m_cubeMesh, shader, camera,
            cubeTransform(glm::vec3(static_cast<float>(line), -0.012F, 0.0F), glm::vec3(0.015F, 0.02F, 40.0F)),
            *m_gridTexture, light);
        renderer.draw(*m_cubeMesh, shader, camera,
            cubeTransform(glm::vec3(0.0F, -0.01F, static_cast<float>(line)), glm::vec3(40.0F, 0.02F, 0.015F)),
            *m_gridTexture, light);
    }
    renderer.draw(*m_cubeMesh, shader, camera, cubeTransform(glm::vec3(10, 0.01F, 0), glm::vec3(20, 0.04F, 0.04F)), *m_xAxisTexture, light);
    renderer.draw(*m_cubeMesh, shader, camera, cubeTransform(glm::vec3(0, 10, 0), glm::vec3(0.04F, 20, 0.04F)), *m_yAxisTexture, light);
    renderer.draw(*m_cubeMesh, shader, camera, cubeTransform(glm::vec3(0, 0.01F, 10), glm::vec3(0.04F, 0.04F, 20)), *m_zAxisTexture, light);

    renderPieces(renderer, shader, skeletalShader, camera, light);

    const int previewMaterial = PIECES[m_selectedIndex] == BuildPieceType::Floor
        ? m_floorMaterialIndex : PIECES[m_selectedIndex] == BuildPieceType::Wall
        ? m_wallMaterialIndex : 0;
    const BuildPiece preview{
        PIECES[m_selectedIndex], m_previewPosition, m_yawDegrees,
        m_wallVariant, previewMaterial, m_objectScale };
    renderPiece(preview, renderer, shader, camera, light, m_previewTexture.get());
}

void WorldBuilder::renderPieces(const Renderer& renderer, const Shader& shader,
    const Shader& skeletalShader, const Camera& camera, const Light& light,
    const bool includeDynamicObjects) const
{
    std::size_t dummy = 0;
    for (const BuildPiece& piece : m_pieces)
    {
        if (piece.type == BuildPieceType::Dummy && dummy < m_dummyVisuals.size())
            m_dummyVisuals[dummy++]->render(renderer, skeletalShader, camera, light);
        else if (piece.type == BuildPieceType::Door)
            continue;
        else if (!includeDynamicObjects
            && (piece.type == BuildPieceType::Cube || piece.type == BuildPieceType::Sphere))
            continue;
        else
            renderPiece(piece, renderer, shader, camera, light);
    }
    for (const std::unique_ptr<Door>& door : m_doorVisuals)
        door->render(renderer, shader, camera, light);
}

void WorldBuilder::renderPiece(const BuildPiece& piece, const Renderer& renderer,
    const Shader& shader, const Camera& camera, const Light& light,
    const Texture* overrideTexture) const
{
    if (piece.type == BuildPieceType::Stairs && m_stairModel != nullptr)
    {
        m_stairModel->render(renderer, shader, camera, pieceTransform(piece), light);
        return;
    }
    if (piece.type == BuildPieceType::Sphere)
    {
        renderer.draw(*m_sphereMesh, shader, camera, pieceTransform(piece),
            overrideTexture != nullptr ? *overrideTexture : *m_sphereTexture, light);
        return;
    }
    const Texture& texture = overrideTexture != nullptr ? *overrideTexture
        : piece.type == BuildPieceType::Floor
            ? *m_floorTextures[static_cast<std::size_t>(piece.materialIndex) % m_floorTextures.size()]
        : piece.type == BuildPieceType::Door ? *m_doorTexture
        : piece.type == BuildPieceType::Cube ? *m_cubeTexture
            : *m_wallTextures[static_cast<std::size_t>(piece.materialIndex) % m_wallTextures.size()];
    if (piece.type != BuildPieceType::Wall || piece.wallVariant == WallVariant::Solid)
    {
        renderer.draw(*m_cubeMesh, shader, camera, pieceTransform(piece), texture, light);
        return;
    }

    auto drawWallBox = [&](const glm::vec3& localCenter, const glm::vec3& scale)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0F), piece.position);
        transform = glm::rotate(transform, glm::radians(piece.yawDegrees), glm::vec3(0, 1, 0));
        transform = glm::translate(transform, localCenter);
        transform = glm::scale(transform, scale);
        renderer.draw(*m_cubeMesh, shader, camera, transform, texture, light);
    };
    if (piece.wallVariant == WallVariant::DoorFrame)
    {
        drawWallBox(glm::vec3(-0.75F, 1.25F, 0), glm::vec3(0.5F, 2.5F, 0.1F));
        drawWallBox(glm::vec3(0.75F, 1.25F, 0), glm::vec3(0.5F, 2.5F, 0.1F));
        drawWallBox(glm::vec3(0, 2.25F, 0), glm::vec3(1.0F, 0.5F, 0.1F));
    }
    else
    {
        drawWallBox(glm::vec3(-0.75F, 1.25F, 0), glm::vec3(0.5F, 2.5F, 0.1F));
        drawWallBox(glm::vec3(0.75F, 1.25F, 0), glm::vec3(0.5F, 2.5F, 0.1F));
        drawWallBox(glm::vec3(0, 0.4F, 0), glm::vec3(1.0F, 0.8F, 0.1F));
        drawWallBox(glm::vec3(0, 2.1F, 0), glm::vec3(1.0F, 0.8F, 0.1F));
    }
}

void WorldBuilder::renderUi(const TextRenderer& text, const Shader& shader,
    const int width, const int height) const
{
    const float top = static_cast<float>(height) - 55.0F;
    text.renderText(shader, "WORLD BUILDER", 20, top, 0.62F, glm::vec3(0.95F));
    text.renderText(shader, "F1 Exit | Tab Pieces | G Snap/Free | R Rotate | C Wall Type | T Material",
        20, top - 35, 0.36F, glm::vec3(0.75F));
    text.renderText(shader, "LMB Place | RMB Remove | [ / ] Object Scale | Ctrl+S Save | Ctrl+L Load",
        20, top - 60, 0.36F, glm::vec3(0.75F));
    std::ostringstream state;
    state << "Mode: " << (m_snapEnabled ? "GRID SNAP (0.5)" : "FREE")
        << "  Piece: " << pieceName(PIECES[m_selectedIndex])
        << "  Height: " << std::fixed << std::setprecision(1) << m_placementHeight;
    if (PIECES[m_selectedIndex] == BuildPieceType::Wall)
        state << "  Type: " << wallVariantName();
    if (PIECES[m_selectedIndex] == BuildPieceType::Floor
        || PIECES[m_selectedIndex] == BuildPieceType::Wall)
        state << "  Material: " << materialName();
    if (PIECES[m_selectedIndex] == BuildPieceType::Cube
        || PIECES[m_selectedIndex] == BuildPieceType::Sphere)
        state << "  Scale: " << std::fixed << std::setprecision(2) << m_objectScale;
    text.renderText(shader, state.str(), 20, top - 92, 0.42F, glm::vec3(1.0F, 0.78F, 0.2F));
    text.renderText(shader, m_status, 20, 25, 0.38F, glm::vec3(0.65F, 0.9F, 0.7F));
    if (m_menuOpen)
    {
        text.renderText(shader, "PIECE LIBRARY", 55, top - 145, 0.55F, glm::vec3(0.9F));
        text.renderText(shader, "ARCHITECTURE", 55, top - 180, 0.38F, glm::vec3(0.4F, 0.7F, 1.0F));
        for (int index = 0; index < 4; ++index)
            text.renderText(shader, std::string(index == m_selectedIndex ? "> " : "  ") + pieceName(PIECES[index]),
                70, top - 215 - index * 30.0F, 0.42F,
                index == m_selectedIndex ? glm::vec3(1.0F, 0.8F, 0.2F) : glm::vec3(0.85F));
        text.renderText(shader, "OBJECTS", 55, top - 345, 0.38F, glm::vec3(0.4F, 0.7F, 1.0F));
        for (int index = 4; index < 6; ++index)
            text.renderText(shader, std::string(index == m_selectedIndex ? "> " : "  ") + pieceName(PIECES[index]),
                70, top - 380 - (index - 4) * 30.0F, 0.42F,
                index == m_selectedIndex ? glm::vec3(1.0F, 0.8F, 0.2F) : glm::vec3(0.85F));
        text.renderText(shader, "ENTITIES", 55, top - 450, 0.38F, glm::vec3(0.4F, 0.7F, 1.0F));
        text.renderText(shader, std::string(m_selectedIndex == 6 ? "> " : "  ") + "Dummy",
            70, top - 485, 0.42F, m_selectedIndex == 6 ? glm::vec3(1.0F, 0.8F, 0.2F) : glm::vec3(0.85F));
        text.renderText(shader, "Up/Down Navigate | Enter Select", 55, top - 530, 0.34F, glm::vec3(0.7F));
    }
    (void)width;
}

void WorldBuilder::rebuildDummies()
{
    m_dummyVisuals.clear();
    if (m_personModel == nullptr) return;
    for (const BuildPiece& piece : m_pieces)
        if (piece.type == BuildPieceType::Dummy)
            m_dummyVisuals.push_back(std::make_unique<Person>(m_personModel,
                piece.position, piece.position, 0.8F));
}

void WorldBuilder::rebuildDoors()
{
    m_doorVisuals.clear();
    for (const BuildPiece& piece : m_pieces)
    {
        if (piece.type != BuildPieceType::Door) continue;
        const float radians = glm::radians(piece.yawDegrees);
        const glm::vec3 hingeOffset(
            -0.5F * std::cos(radians), 0.0F, 0.5F * std::sin(radians));
        m_doorVisuals.push_back(std::make_unique<Door>(
            piece.position + hingeOffset, 1.0F, 2.0F, 0.075F,
            DOOR_TEXTURE, piece.yawDegrees));
    }
}

void WorldBuilder::rebuildColliders()
{
    m_colliders.clear();
    for (const BuildPiece& piece : m_pieces)
    {
        if (piece.type == BuildPieceType::Dummy || piece.type == BuildPieceType::Door) continue;
        if (piece.type == BuildPieceType::Floor)
            m_colliders.push_back(rotatedBox(piece.position,
                glm::vec3(0, 0.05F, 0), glm::vec3(1.0F, 0.05F, 1.0F), piece.yawDegrees));
        else if (piece.type == BuildPieceType::Wall)
        {
            auto add = [&](glm::vec3 center, glm::vec3 half)
            { m_colliders.push_back(rotatedBox(piece.position, center, half, piece.yawDegrees)); };
            if (piece.wallVariant == WallVariant::Solid)
                add(glm::vec3(0, 1.25F, 0), glm::vec3(1.0F, 1.25F, 0.05F));
            else if (piece.wallVariant == WallVariant::DoorFrame)
            {
                add(glm::vec3(-0.75F, 1.25F, 0), glm::vec3(0.25F, 1.25F, 0.05F));
                add(glm::vec3(0.75F, 1.25F, 0), glm::vec3(0.25F, 1.25F, 0.05F));
                add(glm::vec3(0, 2.25F, 0), glm::vec3(0.5F, 0.25F, 0.05F));
            }
            else
            {
                add(glm::vec3(-0.75F, 1.25F, 0), glm::vec3(0.25F, 1.25F, 0.05F));
                add(glm::vec3(0.75F, 1.25F, 0), glm::vec3(0.25F, 1.25F, 0.05F));
                add(glm::vec3(0, 0.4F, 0), glm::vec3(0.5F, 0.4F, 0.05F));
                add(glm::vec3(0, 2.1F, 0), glm::vec3(0.5F, 0.4F, 0.05F));
            }
        }
        else if (piece.type == BuildPieceType::Stairs)
        {
            constexpr int count = 16;
            for (int step = 0; step < count; ++step)
            {
                const float depth = 4.0F / count;
                const float height = 2.95F * static_cast<float>(step + 1) / count;
                m_colliders.push_back(rotatedBox(piece.position,
                    glm::vec3(0, height * 0.5F, -2.0F + depth * (step + 0.5F)),
                    glm::vec3(0.8F, height * 0.5F, depth * 0.5F), piece.yawDegrees));
            }
        }
        // Cube and sphere objects are represented by dynamic PhysX actors
        // during gameplay, so they must not also get static colliders.
    }
}

void WorldBuilder::save()
{
    const std::filesystem::path path = savePath();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file) { m_status = "Save failed"; return; }
    file << "FPSWORLD 4\n";
    for (const BuildPiece& piece : m_pieces)
        file << static_cast<int>(piece.type) << ' ' << piece.position.x << ' '
            << piece.position.y << ' ' << piece.position.z << ' ' << piece.yawDegrees
            << ' ' << static_cast<int>(piece.wallVariant)
            << ' ' << piece.materialIndex << ' ' << piece.scale << '\n';
    m_status = std::string("Saved ") + std::to_string(m_pieces.size()) + " pieces to worlds/editor_world.world";
}

void WorldBuilder::load()
{
    const std::filesystem::path path = savePath();
    std::ifstream file(path);
    std::string header;
    int version = 0;
    if (!(file >> header >> version) || header != "FPSWORLD")
    { m_status = "No valid world at worlds/editor_world.world"; return; }
    std::vector<BuildPiece> loaded;
    int type = 0;
    BuildPiece piece;
    int variant = 0;
    int material = 0;
    float scale = 1.0F;
    while (file >> type >> piece.position.x >> piece.position.y >> piece.position.z >> piece.yawDegrees)
    {
        if (version >= 2) file >> variant;
        if (version >= 3) file >> material;
        if (version >= 4) file >> scale;
        if (version < 3 && type == 3) type = static_cast<int>(BuildPieceType::Dummy);
        piece.wallVariant = static_cast<WallVariant>(std::clamp(variant, 0, 2));
        piece.materialIndex = std::max(material, 0);
        piece.scale = std::clamp(scale, 0.1F, 10.0F);
        if (type >= 0 && type <= 6)
        { piece.type = static_cast<BuildPieceType>(type); loaded.push_back(piece); }
    }
    m_pieces = std::move(loaded);
    rebuildDummies();
    rebuildDoors();
    rebuildColliders();
    m_status = std::string("Loaded ") + std::to_string(m_pieces.size()) + " pieces";
}

void WorldBuilder::syncDynamicObjects(RigidBodyWorld& rigidBodies) const
{
    std::vector<RigidBodyWorld::WorldBuilderObject> objects;
    for (const BuildPiece& piece : m_pieces)
    {
        if (piece.type != BuildPieceType::Cube && piece.type != BuildPieceType::Sphere)
            continue;
        objects.push_back(RigidBodyWorld::WorldBuilderObject{
            piece.position, piece.scale, piece.type == BuildPieceType::Sphere });
    }
    rigidBodies.setWorldBuilderObjects(objects);
}

const char* WorldBuilder::wallVariantName() const
{
    switch (m_wallVariant)
    {
    case WallVariant::Solid: return "Solid";
    case WallVariant::DoorFrame: return "Door Frame";
    case WallVariant::Window: return "Window";
    }
    return "Solid";
}

const char* WorldBuilder::materialName() const
{
    if (PIECES[m_selectedIndex] == BuildPieceType::Floor)
        return m_floorMaterialIndex == 0 ? "Flooring" : "Concrete";
    if (PIECES[m_selectedIndex] == BuildPieceType::Wall)
        return m_wallMaterialIndex == 0 ? "Wallpaper" : "Plaster";
    return "Default";
}

const char* WorldBuilder::pieceName(const BuildPieceType type) const
{
    switch (type)
    {
    case BuildPieceType::Floor: return "Floor";
    case BuildPieceType::Wall: return "Wall";
    case BuildPieceType::Stairs: return "Stairs";
    case BuildPieceType::Door: return "Door";
    case BuildPieceType::Dummy: return "Dummy";
    case BuildPieceType::Cube: return "Cube";
    case BuildPieceType::Sphere: return "Sphere";
    }
    return "Unknown";
}
