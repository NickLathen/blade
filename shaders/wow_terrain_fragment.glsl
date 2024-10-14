#version 320 es
precision highp float;

#include "wow_terrain_functions.glsl"

vec3 blendColors(vec3 baseColor, vec3 blendColor, float alpha) {
	return baseColor + alpha * (blendColor - baseColor);
};

struct Slots {
    uint slot[4];
};

layout(std430, binding=1) buffer uTextureSlotBuffer {
  Slots uTextureSlots[256];
};

layout(std430, binding=2) buffer uAlphaSlotBuffer {
  Slots uAlphaSlots[256];
};

uniform highp sampler2DArray uBlendTexture;
uniform highp sampler2DArray uAlphaTexture;
uniform highp sampler2DArray uShadowTexture;

uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uCameraPos;

in vec3 worldPos;
in vec2 texCoords;
in vec3 normal;
flat in int blockNumber;
out vec4 FragColor;

void main() {
  vec2 scaledCoords = texCoords * 8.0;
  float alphaScale = 63.0 / 64.0; //see MCNK.header do_not_fix_alpha_map
  vec2 alphaCoords = texCoords * alphaScale + (1.0 - alphaScale) / 2.0;
  vec3 color = texture(uBlendTexture, vec3(scaledCoords, uTextureSlots[blockNumber].slot[0])).rgb;
  uint numTextures = uAlphaSlots[blockNumber].slot[0];
  for(uint i = 1u; i < numTextures; i++) {
    vec3 blendColor = texture(uBlendTexture, vec3(scaledCoords, uTextureSlots[blockNumber].slot[i])).rgb;
    float alpha = texture(uAlphaTexture, vec3(alphaCoords, uAlphaSlots[blockNumber].slot[i])).r;
    color = blendColors(color, blendColor, alpha);
  }
  
  vec3 nNormalDir = normalize(normal);
  vec3 lightDir = normalize(uLightDir);

  //Lighting
  float diffuseFactor = dot(nNormalDir, lightDir);
  vec3 diffuseColor = max(diffuseFactor, 0.0) *
                      uLightColor;
  vec3 ambientColor = uAmbientLightColor * color.rgb;
  vec3 litColor = diffuseColor * color.rgb;
  litColor *= GetShadowFactor(uShadowTexture, texCoords, blockNumber);
  color = ambientColor + litColor;
  FragColor = vec4(color, 1.0);
};
