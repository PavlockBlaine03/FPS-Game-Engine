#pragma once

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader
{
public:
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    [[nodiscard]] static Shader fromFiles(
        const std::string& vertexPath,
        const std::string& fragmentPath);

    void use() const;

    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const;

    [[nodiscard]] unsigned int id() const { return m_programId; }

private:
    static unsigned int compile(unsigned int shaderType, const char* source);

    unsigned int m_programId = 0;
};