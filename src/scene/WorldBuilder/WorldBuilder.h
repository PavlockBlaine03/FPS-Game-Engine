#pragma once

#include "physics/AABB.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

class Camera;
class InputManager;
struct Light;
class Mesh;
class Model;
class Person;
class Door;
class Renderer;
class RigidBodyWorld;
class Shader;
class SkeletalModel;
class TextRenderer;
class Texture;

enum class BuildPieceType
{
    Floor,
    Wall,
    Stairs,
    Door,
    Dummy,
    Cube,
    Sphere
};

enum class WallVariant
{
    Solid,
    DoorFrame,
    Window
};

struct BuildPiece
{
    BuildPieceType type = BuildPieceType::Floor;
    glm::vec3 position{ 0.0F };
    float yawDegrees = 0.0F;
    WallVariant wallVariant = WallVariant::Solid;
    int materialIndex = 0;
    float scale = 1.0F;
};

class WorldBuilder
{
public:
    explicit WorldBuilder(std::shared_ptr<SkeletalModel> personModel);
    ~WorldBuilder();

    WorldBuilder(const WorldBuilder&) = delete;
    WorldBuilder& operator=(const WorldBuilder&) = delete;

    void update(const InputManager& input, Camera& camera, float deltaTime);
    void renderWorld(const Renderer& renderer, const Shader& shader,
        const Shader& skeletalShader, const Camera& camera, const Light& light) const;
    void renderUi(const TextRenderer& textRenderer, const Shader& textShader,
        int width, int height) const;

    [[nodiscard]] bool menuOpen() const { return m_menuOpen; }
    [[nodiscard]] bool hasPieces() const { return !m_pieces.empty(); }
    [[nodiscard]] const std::vector<AABB>& colliders() const { return m_colliders; }
    [[nodiscard]] std::vector<std::unique_ptr<Person>>& dummies() { return m_dummyVisuals; }
    [[nodiscard]] const std::vector<std::unique_ptr<Person>>& dummies() const { return m_dummyVisuals; }
    [[nodiscard]] std::vector<std::unique_ptr<Door>>& doors() { return m_doorVisuals; }
    [[nodiscard]] const std::vector<std::unique_ptr<Door>>& doors() const { return m_doorVisuals; }
    void renderPieces(const Renderer& renderer, const Shader& shader,
        const Shader& skeletalShader, const Camera& camera, const Light& light,
        bool includeDynamicObjects = true) const;
    void syncDynamicObjects(RigidBodyWorld& rigidBodies) const;

private:
    void placePiece();
    void removePiece();
    void rebuildDummies();
    void rebuildDoors();
    void rebuildColliders();
    void renderPiece(const BuildPiece& piece, const Renderer& renderer,
        const Shader& shader, const Camera& camera, const Light& light,
        const Texture* overrideTexture = nullptr) const;
    void save();
    void load();
    [[nodiscard]] glm::mat4 pieceTransform(const BuildPiece& piece) const;
    [[nodiscard]] const char* pieceName(BuildPieceType type) const;
    [[nodiscard]] const char* wallVariantName() const;
    [[nodiscard]] const char* materialName() const;

    std::shared_ptr<SkeletalModel> m_personModel;
    std::unique_ptr<Mesh> m_cubeMesh;
    std::unique_ptr<Mesh> m_sphereMesh;
    std::unique_ptr<Model> m_stairModel;
    std::vector<std::unique_ptr<Texture>> m_floorTextures;
    std::vector<std::unique_ptr<Texture>> m_wallTextures;
    std::unique_ptr<Texture> m_doorTexture;
    std::unique_ptr<Texture> m_cubeTexture;
    std::unique_ptr<Texture> m_sphereTexture;
    std::unique_ptr<Texture> m_gridTexture;
    std::unique_ptr<Texture> m_xAxisTexture;
    std::unique_ptr<Texture> m_yAxisTexture;
    std::unique_ptr<Texture> m_zAxisTexture;
    std::unique_ptr<Texture> m_previewTexture;
    std::vector<BuildPiece> m_pieces;
    std::vector<std::unique_ptr<Person>> m_dummyVisuals;
    std::vector<std::unique_ptr<Door>> m_doorVisuals;
    std::vector<AABB> m_colliders;
    glm::vec3 m_previewPosition{ 0.0F };
    float m_placementHeight = 0.0F;
    float m_yawDegrees = 0.0F;
    int m_selectedIndex = 0;
    bool m_menuOpen = true;
    bool m_snapEnabled = true;
    WallVariant m_wallVariant = WallVariant::Solid;
    int m_floorMaterialIndex = 0;
    int m_wallMaterialIndex = 0;
    float m_objectScale = 1.0F;
    std::string m_status = "New editor world";
};
