#version 330 core

in vec2 texCoords;
out vec4 fragmentColor;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    float alpha = texture(text, texCoords).r;
    fragmentColor = vec4(textColor, alpha);
}