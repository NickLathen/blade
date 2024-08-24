#version 300 es

uniform mat4 uMVP;
uniform mat3 uNormalMatrix;
uniform vec3 uColor;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
out vec4 color;
out vec3 normal;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    color = vec4(uColor, 1.0);
    normal = normalize(uNormalMatrix * aNormal);
}
