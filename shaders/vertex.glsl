#version 300 es

uniform mat4 uMVP;
uniform mat3 uWorldMatrix;
uniform mat4 uLightMVP;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in uint aMaterialIdx;
out vec3 normalDir;
out vec3 worldPos;
flat out uint materialIdx;
out vec4 lightSpacePosition;
void main() {
    normalDir = uWorldMatrix * aNormal;
    worldPos = uWorldMatrix * aPos;
    materialIdx = aMaterialIdx;
    gl_Position = uMVP * vec4(aPos, 1.0);
    lightSpacePosition = uLightMVP * vec4(aPos, 1.0);
}
