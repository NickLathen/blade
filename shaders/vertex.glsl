#version 300 es

uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat4 uCameraTransform;
uniform mat4 uLightMVP;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in uint aMaterialIdx;
layout (location = 3) in vec2 aTexCoords;
out vec3 normalDir;
out vec3 worldPos;
out vec2 texCoords;
flat out uint materialIdx;
out vec4 lightSpacePosition;
void main() {
    normalDir = mat3(uModelMatrix) * aNormal;
    worldPos = (uModelMatrix * vec4(aPos, 1.0)).xyz;
    materialIdx = aMaterialIdx;
    texCoords = aTexCoords;
    gl_Position = uMVP * vec4(aPos, 1.0);
    lightSpacePosition = uLightMVP * vec4(aPos, 1.0);
}
 