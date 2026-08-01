#include "rendering/Renderer.h"
#include "rendering/SkinnedMesh.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_inverse.hpp>

Renderer::Renderer(const int width, const int height)
    : m_width(width)
    , m_height(height)
{
    glEnable(GL_DEPTH_TEST);
}

void Renderer::beginFrame() const
{
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.08F, 0.10F, 0.14F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw(
    const Mesh& mesh,
    const Shader& shader,
    const Camera& camera,
    const glm::mat4& model,
    const Texture& texture,
    const Light& light) const
{
    shader.use();

    const float aspectRatio =
        static_cast<float>(m_width) / static_cast<float>(m_height);

    shader.setMat4("view", camera.viewMatrix());
    shader.setMat4("projection", camera.projectionMatrix(aspectRatio));
    shader.setMat4("model", model);

    // Normal matrix: transposed inverse of the model matrix's upper-left
    // 3x3, needed so normals stay correct under non-uniform scaling
    // (without this, scaled geometry would light incorrectly).
    const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));
    shader.setMat3("normalMatrix", normalMatrix);

    shader.setVec3("lightPosition", light.position);
    shader.setVec3("lightColor", light.color);
    shader.setVec3("viewPosition", camera.position());
    shader.setFloat("ambientStrength", light.ambientStrength);
    shader.setFloat("specularStrength", light.specularStrength);
    shader.setFloat("shininess", light.shininess);

    texture.bind(0);

    mesh.draw();
}

void Renderer::resize(const int width, const int height)
{
    m_width = width;
    m_height = height;
}

void Renderer::drawSkinned(
    const SkinnedMesh& mesh,
    const Shader& shader,
    const Camera& camera,
    const glm::mat4& model,
    const Texture& texture,
    const Light& light,
    const std::vector<glm::mat4>& boneMatrices) const
{
    shader.use();
    const float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    shader.setMat4("view", camera.viewMatrix());
    shader.setMat4("projection", camera.projectionMatrix(aspectRatio));
    shader.setMat4("model", model);
    shader.setMat3("normalMatrix", glm::inverseTranspose(glm::mat3(model)));
    shader.setVec3("lightPosition", light.position);
    shader.setVec3("lightColor", light.color);
    shader.setVec3("viewPosition", camera.position());
    shader.setFloat("ambientStrength", light.ambientStrength);
    shader.setFloat("specularStrength", light.specularStrength);
    shader.setFloat("shininess", light.shininess);
    shader.setInt("diffuseTexture", 0);
    shader.setInt("boneMatrices", 1);

    texture.bind(0);
    mesh.uploadBoneMatrices(boneMatrices);
    mesh.draw();
}
