#include "rendering/TextRenderer.h"
#include "rendering/Shader.h"

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <stdexcept>

TextRenderer::TextRenderer(const std::string& fontPath, const unsigned int fontSize)
{
    FT_Library freeTypeLibrary{};

    if (FT_Init_FreeType(&freeTypeLibrary) != 0)
    {
        throw std::runtime_error("Failed to initialize FreeType.");
    }

    FT_Face face{};

    if (FT_New_Face(freeTypeLibrary, fontPath.c_str(), 0, &face) != 0)
    {
        FT_Done_FreeType(freeTypeLibrary);
        throw std::runtime_error("Failed to load font: " + fontPath);
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; ++c)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0)
        {
            std::cerr << "Failed to load glyph for character: " << c << '\n';
            continue;
        }

        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            static_cast<int>(face->glyph->bitmap.width),
            static_cast<int>(face->glyph->bitmap.rows),
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        m_characters[c] = Character{
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(freeTypeLibrary);

    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);

    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

TextRenderer::~TextRenderer()
{
    for (const Character& character : m_characters)
    {
        if (character.textureId != 0)
        {
            glDeleteTextures(1, &character.textureId);
        }
    }

    glDeleteVertexArrays(1, &m_vertexArrayObject);
    glDeleteBuffers(1, &m_vertexBufferObject);
}

void TextRenderer::renderText(
    const Shader& shader,
    const std::string& text,
    float x,
    const float y,
    const float scale,
    const glm::vec3& color) const
{
    shader.use();
    glUniform3f(
        glGetUniformLocation(shader.id(), "textColor"),
        color.x,
        color.y,
        color.z
    );

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vertexArrayObject);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    for (const char c : text)
    {
        const Character& ch = m_characters[static_cast<unsigned char>(c)];

        const float xPos = x + static_cast<float>(ch.bearing.x) * scale;
        const float yPos = y - static_cast<float>(ch.size.y - ch.bearing.y) * scale;

        const float w = static_cast<float>(ch.size.x) * scale;
        const float h = static_cast<float>(ch.size.y) * scale;

        const float vertices[6][4] = {
            { xPos,     yPos + h, 0.0F, 0.0F },
            { xPos,     yPos,     0.0F, 1.0F },
            { xPos + w, yPos,     1.0F, 1.0F },

            { xPos,     yPos + h, 0.0F, 0.0F },
            { xPos + w, yPos,     1.0F, 1.0F },
            { xPos + w, yPos + h, 1.0F, 0.0F }
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureId);

        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += static_cast<float>(ch.advance >> 6) * scale;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::renderCrosshair(
    const Shader& shader,
    const int screenWidth,
    const int screenHeight,
    const float scale,
    const glm::vec3& color) const
{
    const Character& crosshair = m_characters[static_cast<unsigned char>('+')];
    const float centerX = static_cast<float>(screenWidth) * 0.5F;
    const float centerY = static_cast<float>(screenHeight) * 0.5F;

    // renderText positions glyphs from their baseline. Account for the
    // glyph's bearing and bitmap dimensions so the visible '+' itself is
    // centered on the aiming point.
    const float x = centerX
        - (static_cast<float>(crosshair.size.x) * scale * 0.5F)
        - (static_cast<float>(crosshair.bearing.x) * scale);
    const float y = centerY
        + (static_cast<float>(crosshair.size.y) * scale * 0.5F)
        - (static_cast<float>(crosshair.bearing.y) * scale);

    renderText(shader, "+", x, y, scale, color);
}
