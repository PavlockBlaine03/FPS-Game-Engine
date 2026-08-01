#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in ivec4 boneIds;
layout (location = 4) in vec4 boneWeights;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform mat3 normalMatrix;
uniform samplerBuffer boneMatrices;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPosition;

mat4 boneMatrix(int boneId)
{
    int firstTexel = boneId * 4;
    return mat4(
        texelFetch(boneMatrices, firstTexel),
        texelFetch(boneMatrices, firstTexel + 1),
        texelFetch(boneMatrices, firstTexel + 2),
        texelFetch(boneMatrices, firstTexel + 3));
}

void main()
{
    mat4 skin = boneMatrix(boneIds.x) * boneWeights.x
        + boneMatrix(boneIds.y) * boneWeights.y
        + boneMatrix(boneIds.z) * boneWeights.z
        + boneMatrix(boneIds.w) * boneWeights.w;

    vec4 localPosition = skin * vec4(position, 1.0);
    vec4 worldPosition = model * localPosition;
    gl_Position = projection * view * worldPosition;

    fragTexCoord = texCoord;
    fragNormal = normalMatrix * mat3(skin) * normal;
    fragPosition = worldPosition.xyz;
}
