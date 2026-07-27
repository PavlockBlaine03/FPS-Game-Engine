#version 330 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;

out vec4 fragmentColor;

uniform sampler2D diffuseTexture;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 viewPosition;

uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

void main()
{
	vec3 normal = normalize(fragNormal);
	vec3 lightVector = lightPosition - fragPosition;
	float distance = length(lightVector);
	vec3 lightDirection = lightVector / distance;
	vec3 viewDirection = normalize(viewPosition - fragPosition);
	vec3 halfwayDirection = normalize(lightDirection + viewDirection);

	// Distance attenuation: keeps nearby surfaces bright and far surfaces
	// dim, instead of every surface receiving roughly equal light
	// regardless of distance. Constants tuned for a small room scale.
	float attenuation = 1.0 / (1.0 + 0.15 * distance + 0.05 * distance * distance);

	vec3 ambient = ambientStrength * lightColor;

	float diffuseFactor = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = diffuseFactor * lightColor * attenuation;

	float specularFactor = pow(max(dot(normal, halfwayDirection), 0.0), shininess);
	vec3 specular = specularStrength * specularFactor * lightColor * attenuation;

	vec4 textureColor = texture(diffuseTexture, fragTexCoord);

	vec3 lighting = ambient + diffuse + specular;
	fragmentColor = vec4(lighting * textureColor.rgb, textureColor.a);
}