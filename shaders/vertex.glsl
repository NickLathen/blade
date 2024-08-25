#version 300 es

uniform mat4 uMVP;
uniform mat3 uWorldMatrix;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
out vec3 normalDir;
out vec3 worldPos;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    normalDir = normalize(uWorldMatrix * aNormal);
    worldPos = uWorldMatrix * aPos;
}
