#pragma once

#include "rendering/Camera.h"
#include "rendering/Light.h"
#include "rendering/Mesh.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "physics/AABB.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

class Model
{
public:
    explicit Model(const std::string& path);
    ~Model();

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    void render(
        const Renderer& renderer,
        const Shader& shader,
        const Camera& camera,
        const glm::mat4& model,
        const Light& light) const;

    [[nodiscard]] const AABB& localBounds() const { return m_localBounds; }

private:
    struct SubMesh
    {
        std::unique_ptr<Mesh> mesh;
        std::shared_ptr<Texture> texture;
    };

    std::vector<SubMesh> m_subMeshes;
    AABB m_localBounds{ glm::vec3(0.0F), glm::vec3(0.0F) };
};
