#version 300 es
precision highp float;

uniform vec3 uLightPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(uLightPos, 1.0);
    gl_PointSize = 16.0f;
}

