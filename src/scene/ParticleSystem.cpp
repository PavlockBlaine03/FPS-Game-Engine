#include "scene/ParticleSystem.h"
#include "util/MeshFactory.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdlib>

namespace
{
    constexpr glm::vec3 PARTICLE_COLOR(1.0F, 0.7F, 0.2F);
    constexpr int PARTICLES_PER_BURST = 10;
    constexpr float PARTICLE_SPEED = 3.0F;
    constexpr float PARTICLE_SIZE = 0.05F;

    // Returns a random float in [-1, 1].
    float randomUnit()
    {
        return (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0F - 1.0F;
    }
}

ParticleSystem::ParticleSystem()
    : m_particleTexture(std::make_unique<Texture>(PARTICLE_COLOR))
{
    const MeshData cubeData = MeshFactory::createCube(1.0F, 1.0F);

    m_particleMesh = std::make_unique<Mesh>(
        cubeData.vertices.data(),
        cubeData.vertices.size(),
        cubeData.indices.data(),
        cubeData.indices.size(),
        8 // position(3) + normal(3) + uv(2)
    );
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::spawnBurst(const glm::vec3& origin, const glm::vec3& direction)
{
    const glm::vec3 normalizedDirection =
        (glm::length(direction) > 0.0001F) ? glm::normalize(direction) : glm::vec3(0.0F, 1.0F, 0.0F);

    for (int i = 0; i < PARTICLES_PER_BURST; ++i)
    {
        // Scatter each particle's velocity in a cone-ish spread around the
        // main direction, biased mostly outward rather than uniformly
        // random in all directions.
        const glm::vec3 randomSpread(randomUnit(), randomUnit(), randomUnit());
        const glm::vec3 velocity =
            (normalizedDirection * 0.7F + randomSpread * 0.3F) * PARTICLE_SPEED;

        Particle particle;
        particle.position = origin;
        particle.velocity = velocity;
        particle.lifetime = 0.0F;
        particle.maxLifetime = 0.2F + (static_cast<float>(std::rand()) / RAND_MAX) * 0.2F;

        m_particles.push_back(particle);
    }
}

void ParticleSystem::update(const float deltaTime)
{
    for (Particle& particle : m_particles)
    {
        particle.position += particle.velocity * deltaTime;
        particle.lifetime += deltaTime;
    }

    m_particles.erase(
        std::remove_if(
            m_particles.begin(),
            m_particles.end(),
            [](const Particle& particle) { return particle.lifetime >= particle.maxLifetime; }),
        m_particles.end());
}

void ParticleSystem::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light) const
{
    for (const Particle& particle : m_particles)
    {
        // Shrink each particle over its lifetime so the burst visibly fades
        // rather than abruptly disappearing.
        const float lifeRatio = particle.lifetime / particle.maxLifetime;
        const float scale = PARTICLE_SIZE * (1.0F - lifeRatio);

        if (scale <= 0.0F)
        {
            continue;
        }

        glm::mat4 model = glm::translate(glm::mat4(1.0F), particle.position);
        model = glm::scale(model, glm::vec3(scale));

        renderer.draw(*m_particleMesh, shader, camera, model, *m_particleTexture, light);
    }
}