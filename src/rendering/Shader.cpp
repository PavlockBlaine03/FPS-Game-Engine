#include "rendering/Shader.h"
#include "util/FileUtils.h"

#include <iostream>
#include <utility>

Shader::Shader(const char* vertexSource, const char* fragmentSource)
{
    const unsigned int vertexShader = compile(GL_VERTEX_SHADER, vertexSource);
    const unsigned int fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource);

    m_programId = glCreateProgram();

    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);

    int success = 0;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);

    if (success == GL_FALSE)
    {
        char infoLog[512]{};
        glGetProgramInfoLog(m_programId, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << '\n';
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    if (m_programId != 0)
    {
        glDeleteProgram(m_programId);
    }
}

Shader::Shader(Shader&& other) noexcept
    : m_programId(std::exchange(other.m_programId, 0))
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (m_programId != 0)
        {
            glDeleteProgram(m_programId);
        }

        m_programId = std::exchange(other.m_programId, 0);
    }

    return *this;
}

Shader Shader::fromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    const std::string vertexSource = FileUtils::readFile(vertexPath);
    const std::string fragmentSource = FileUtils::readFile(fragmentPath);

    return Shader(vertexSource.c_str(), fragmentSource.c_str());
}

void Shader::use() const
{
    glUseProgram(m_programId);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const
{
    const int location = glGetUniformLocation(m_programId, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const
{
    const int location = glGetUniformLocation(m_programId, name.c_str());
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    const int location = glGetUniformLocation(m_programId, name.c_str());
    glUniform3f(location, value.x, value.y, value.z);
}

void Shader::setFloat(const std::string& name, const float value) const
{
    const int location = glGetUniformLocation(m_programId, name.c_str());
    glUniform1f(location, value);
}

unsigned int Shader::compile(const unsigned int shaderType, const char* source)
{
    const unsigned int shader = glCreateShader(shaderType);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        char infoLog[512]{};
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << '\n';
    }

    return shader;
}