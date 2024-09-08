#version 300 es

uniform sampler2D uNoiseTexture;

uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat4 uLightMVP;
uniform float uHeightScale;
uniform float uWidthScale;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in uint aMaterialIdx;

out vec3 worldPos;
out vec2 texCoords;
out vec2 terrainCoords;
out vec4 lightSpacePosition;
flat out uint materialIdx;

void main() {
    texCoords = aTexCoords;
    terrainCoords = texCoords * uWidthScale;
    float height = texture(uNoiseTexture, terrainCoords).r * uHeightScale;
    vec3 position = vec3(aPos.x, height, aPos.z);
    worldPos = (uModelMatrix * vec4(position, 1.0)).xyz;
    materialIdx = aMaterialIdx;
    gl_Position = uMVP * vec4(position, 1.0);
    lightSpacePosition = uLightMVP * vec4(position, 1.0);
}
 