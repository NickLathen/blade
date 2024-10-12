#version 300 es
precision highp float;

uniform sampler2D uTexture;

vec3 blendColors(vec3 baseColor, vec3 blendColor, float alpha) {
	return baseColor + alpha * (blendColor - baseColor);
}

in vec2 texCoord;
out vec4 FragColor;
void main()
{
    vec4 texColor = texture(uTexture, texCoord);
    //blend alpha against checkerboard background
    vec2 tileCoords = texCoord * 17.0;
    vec3 bgColor;
    if (int(tileCoords.x) % 2 != int(tileCoords.y) % 2) {
        bgColor = vec3(.1,.1,.1);
    } else {
        bgColor = vec3(.05,.05,.05);
    }
    vec3 blended = blendColors(bgColor, texColor.rgb, texColor.a);
    FragColor = vec4(blended, 1.0);
}
