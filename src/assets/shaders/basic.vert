#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform mat3 normalMatrix;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPosition;

void main()
{
	vec4 worldPosition = model * vec4(position, 1.0);

	gl_Position = projection * view * worldPosition;

	fragTexCoord = texCoord;
	fragNormal = normalMatrix * normal;
	fragPosition = worldPosition.xyz;
}