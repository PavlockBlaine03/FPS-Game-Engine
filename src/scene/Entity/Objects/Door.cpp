#include "scene/Entity/Objects/Door.h"
#include "rendering/Renderer.h"
#include "util/MeshFactory.h"
#include "physics/AABB.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

Door::Door(
    const glm::vec3& hingePosition,
    const float width,
    const float height,
    const float thickness,
    const std::string& texturePath,
    const float baseYawDegrees)
    : m_hingePosition(hingePosition)
    , m_width(width)
    , m_height(height)
    , m_thickness(thickness)
    , m_baseYawDegrees(baseYawDegrees)
    , m_texture(std::make_unique<Texture>(texturePath))
{
    // A single, non-repeating cube: the door texture should map once
    // across the whole face rather than tiling like the walls do.
    const MeshData cubeData = MeshFactory::createCube(1.0F, 1.0F, true);

    m_mesh = std::make_unique<Mesh>(
        cubeData.vertices.data(),
        cubeData.vertices.size(),
        cubeData.indices.data(),
        cubeData.indices.size(),
        8 // position(3) + normal(3) + uv(2)
    );
}

Door::~Door() = default;

void Door::interact()
{
    const bool currentlyOpenOrOpening = (m_state == State::Open || m_state == State::Opening);

    if (currentlyOpenOrOpening)
    {
        m_targetAngleDegrees = 0.0F;
        m_state = State::Closing;
    }
    else
    {
        m_targetAngleDegrees = m_openAngleDegrees;
        m_state = State::Opening;
    }
}

void Door::update(const float deltaTime)
{
    if (m_state != State::Opening && m_state != State::Closing)
    {
        return;
    }

    const float step = m_angularSpeedDegreesPerSecond * deltaTime;

    if (m_currentAngleDegrees < m_targetAngleDegrees)
    {
        m_currentAngleDegrees = std::min(m_currentAngleDegrees + step, m_targetAngleDegrees);
    }
    else
    {
        m_currentAngleDegrees = std::max(m_currentAngleDegrees - step, m_targetAngleDegrees);
    }

    if (m_currentAngleDegrees == m_targetAngleDegrees)
    {
        m_state = (m_state == State::Opening) ? State::Open : State::Closed;
    }
}

bool Door::isBlocking() const
{
    // The door is always solid; collider() tracks its current swing
    // angle, so it stays accurate whether closed, mid-swing, or fully open.
    return true;
}

AABB Door::collider() const
{
    // Rotate the door's four local-space top-down corners (hinge-relative,
    // in the XZ plane) by the current swing angle, matching the same
    // transform used in render(), then take their min/max to build an
    // axis-aligned box that fully encloses the rotated door at this angle.
    // This tracks the door's real position in every state (closed, open,
    // or mid-swing) rather than artificially blocking the doorway opening.
    const float angleRadians = glm::radians(m_baseYawDegrees - m_currentAngleDegrees);
    const float cosAngle = std::cos(angleRadians);
    const float sinAngle = std::sin(angleRadians);

    const float halfThickness = m_thickness * 0.5F;

    // Corners relative to the hinge, before rotation: the door spans from
    // x=0 (hinge edge) to x=width, and z=-halfThickness to z=+halfThickness.
    const std::array<glm::vec2, 4> localCornersXZ = {
        glm::vec2(0.0F, -halfThickness),
        glm::vec2(0.0F, halfThickness),
        glm::vec2(m_width, -halfThickness),
        glm::vec2(m_width, halfThickness),
    };

    glm::vec2 minXZ(std::numeric_limits<float>::max());
    glm::vec2 maxXZ(std::numeric_limits<float>::lowest());

    for (const glm::vec2& corner : localCornersXZ)
    {
        const glm::vec2 rotated(
            corner.x * cosAngle - corner.y * sinAngle,
            corner.x * sinAngle + corner.y * cosAngle);

        minXZ = glm::min(minXZ, rotated);
        maxXZ = glm::max(maxXZ, rotated);
    }

    const glm::vec3 min(
        m_hingePosition.x + minXZ.x,
        m_hingePosition.y,
        m_hingePosition.z + minXZ.y);

    const glm::vec3 max(
        m_hingePosition.x + maxXZ.x,
        m_hingePosition.y + m_height,
        m_hingePosition.z + maxXZ.y);

    return AABB{ min, max };
}

void Door::render(
    const Renderer& renderer,
    const Shader& shader,
    const Camera& camera,
    const Light& light) const
{
    // Door mesh is a unit cube centered at the origin. To hinge correctly:
    // 1) Shift it so its hinge-side edge sits at the local origin.
    // 2) Rotate around that edge (world-space Y axis) by the current angle.
    // 3) Translate the whole thing to the hinge's world position.
    glm::mat4 model = glm::translate(glm::mat4(1.0F), m_hingePosition);
    model = glm::rotate(model, glm::radians(m_baseYawDegrees - m_currentAngleDegrees),
        glm::vec3(0.0F, 1.0F, 0.0F));
    model = glm::translate(model, glm::vec3(m_width * 0.5F, m_height * 0.5F, 0.0F));
    model = glm::scale(model, glm::vec3(m_width, m_height, m_thickness));

    renderer.draw(*m_mesh, shader, camera, model, *m_texture, light);
}
