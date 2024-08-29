#version 300 es
precision highp float;

uniform sampler2D uTexture;

in vec2 texCoord;
out vec4 FragColor;
void main()
{
    float depth = texture(uTexture, texCoord).x;
    depth = 1.0 - (1.0 - depth) * 5.0;
    FragColor = vec4(depth);
}
