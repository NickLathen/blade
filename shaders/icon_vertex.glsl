#version 300 es
precision highp float;

uniform vec4 uPos;
uniform uint uColor;
flat out vec4 color;
void main() {
    color = unpackUnorm4x8(uColor);
    gl_Position = uPos;
    gl_PointSize = 10.0f;
}

