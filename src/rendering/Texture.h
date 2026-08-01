#pragma once

#include <glm/glm.hpp>

#include <string>
#include <cstddef>

class Texture
{
public:
    explicit Texture(const std::string& path);
    Texture(const unsigned char* encodedData, std::size_t encodedSize);
    explicit Texture(const glm::vec3& solidColor);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void bind(unsigned int unit = 0) const;

    [[nodiscard]] unsigned int id() const { return m_textureId; }

private:
    unsigned int m_textureId = 0;
};
