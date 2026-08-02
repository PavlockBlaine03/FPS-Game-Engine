#pragma once

#include "scene/Entity/Entity.h"
#include "physics/AABB.h"
#include "rendering/Mesh.h"
#include "rendering/Texture.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>

enum class State
{
    Closed,
    Opening,
    Open,
    Closing
};

/*
* A Door class that tracks collision when open and closed
*/
class Door : public Entity
{
public:
    Door(
        const glm::vec3& hingePosition,
        float width,
        float height,
        float thickness,
        const std::string& texturePath,
        float baseYawDegrees = 0.0F);
    ~Door() override;

    Door(const Door&) = delete;
    Door& operator=(const Door&) = delete;

    void update(float deltaTime);

    // Toggles between opening and closing from whatever state it's
    // currently in.
    void interact();

    void render(
        const Renderer& renderer,
        const Shader& shader,
        const Camera& camera,
        const Light& light) const override;
    State& getState() {return m_state;}

    // The door is always solid -- this returns a collider matching its
    // current swing angle (see collider()), whether closed, open, or
    // mid-swing.
    [[nodiscard]] bool isBlocking() const;

    // Axis-aligned collider enclosing the door's footprint at its current
    // rotation angle. Tight when closed/fully open; loosely conservative
    // (slightly larger than the actual rotated door) mid-swing, since AABBs
    // can't represent rotated boxes exactly.
    [[nodiscard]] AABB collider() const;

    [[nodiscard]] const glm::vec3& hingePosition() const { return m_hingePosition; }

private:
    

    glm::vec3 m_hingePosition;
    float m_width;
    float m_height;
    float m_thickness;
    float m_baseYawDegrees;

    State m_state = State::Closed;
    float m_currentAngleDegrees = 0.0F;
    float m_targetAngleDegrees = 0.0F;

    float m_openAngleDegrees = 100.0F;
    float m_angularSpeedDegreesPerSecond = 180.0F;

    std::unique_ptr<Mesh> m_mesh;
    std::unique_ptr<Texture> m_texture;
};
