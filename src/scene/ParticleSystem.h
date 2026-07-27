#pragma once

#include "rendering/Camera.h"
#include "rendering/Light.h"
#include "rendering/Mesh.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class ParticleSystem
{
public:
    ParticleSystem();
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // Spawns a small burst of short-lived particles at `origin`, generally
    // moving outward biased along `direction` (e.g. away from a wall the
    // bullet just hit, or out of the gun's muzzle).
    void spawnBurst(const glm::vec3& origin, const glm::vec3& direction);

    void update(float deltaTime);

    void render(
        const Renderer& renderer,
        const Shader& shader,
        const Camera& camera,
        const Light& light) const;

private:
    struct Particle
    {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime = 0.0F;
        float maxLifetime = 0.3F;
    };

    std::vector<Particle> m_particles;

    std::unique_ptr<Mesh> m_particleMesh;
    std::unique_ptr<Texture> m_particleTexture;
};