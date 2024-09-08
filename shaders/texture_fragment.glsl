#version 300 es
precision highp float;

uniform sampler2DShadow uTexture;

in vec2 texCoord;
out vec4 FragColor;
void main()
{
    float depth = texture(uTexture, vec3(texCoord, 1.0f));
    depth = 1.0 - (1.0 - depth) * 1.0;
    FragColor = vec4(depth);
}
