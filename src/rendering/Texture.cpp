#include "rendering/Texture.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <array>
#include <stdexcept>
#include <utility>

Texture::Texture(const std::string& path)
{
    stbi_set_flip_vertically_on_load(true);

    int width = 0;
    int height = 0;
    int channelCount = 0;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channelCount, 0);

    if (data == nullptr)
    {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    const GLenum format = (channelCount == 4) ? GL_RGBA : GL_RGB;

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<int>(format),
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::Texture(const glm::vec3& solidColor)
{
    const std::array<unsigned char, 3> pixel = {
        static_cast<unsigned char>(solidColor.r * 255.0F),
        static_cast<unsigned char>(solidColor.g * 255.0F),
        static_cast<unsigned char>(solidColor.b * 255.0F)
    };

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        1,
        1,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        pixel.data()
    );

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
    if (m_textureId != 0)
    {
        glDeleteTextures(1, &m_textureId);
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_textureId(std::exchange(other.m_textureId, 0))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_textureId != 0)
        {
            glDeleteTextures(1, &m_textureId);
        }

        m_textureId = std::exchange(other.m_textureId, 0);
    }

    return *this;
}

void Texture::bind(const unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

Texture::Texture(const unsigned char* encodedData, const std::size_t encodedSize)
{
    stbi_set_flip_vertically_on_load(true);
    int width = 0;
    int height = 0;
    int channelCount = 0;
    unsigned char* data = stbi_load_from_memory(
        encodedData, static_cast<int>(encodedSize), &width, &height, &channelCount, 0);
    if (data == nullptr)
    {
        throw std::runtime_error("Failed to decode embedded model texture");
    }

    const GLenum format = (channelCount == 4) ? GL_RGBA : GL_RGB;
    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<int>(format), width, height, 0,
        format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}
