#pragma once

#include <glm/glm.hpp>
#include <array>
#include <string>

class Shader;

class TextRenderer
{
public:
	TextRenderer(const std::string& findPath, unsigned int fontSize);
	~TextRenderer();

	TextRenderer(const TextRenderer&) = delete;
	TextRenderer& operator=(const TextRenderer&) = delete;

	void renderText(
		const Shader& shader,
		const std::string& text,
		float x,
		float y,
		float scale,
		const glm::vec3& color) const;

private:
	struct Character
	{
		unsigned int textureId = 0;
		glm::ivec2 size{ 0, 0 };
		glm::ivec2 bearing{ 0, 0 };
		unsigned int advance = 0;
	};

	std::array<Character, 128> m_characters{};

	unsigned int m_vertexArrayObject = 0;
	unsigned int m_vertexBufferObject = 0;
};