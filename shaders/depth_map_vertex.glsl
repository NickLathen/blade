#version 300 es
precision highp float;

uniform mat4 uMVP;
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position =  uMVP * vec4(aPos, 1.0);
}

