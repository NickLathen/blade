#version 300 es
precision highp float;

#include "structs.glsl"
#include "functions.glsl"
#include "terrain_functions.glsl"

uniform sampler2DShadow uDepthTexture;
uniform sampler2D uNoiseTexture;
uniform sampler2D uHeightmapTexture;
uniform sampler2DArray uBlendTexture;

uniform mat4 uModelMatrix;
uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uCameraPos;
uniform float uSpecularPower;
uniform float uShininessScale;

#define NUM_MATERIALS 256
layout(std140) uniform uMaterialBlock {
  Material materials[NUM_MATERIALS];
} uMaterial;

layout(std140) uniform uTileConfigBlock {
  TileConfig tileConfig;
} uTileConfig;

in vec3 worldPos;
in vec2 texCoords;
in vec2 heightmapCoords;
in vec4 lightSpacePosition;
flat in uint materialIdx;
out vec4 FragColor;

void main() {
  TileConfig tc = uTileConfig.tileConfig;
  Material material = uMaterial.materials[materialIdx];

  float coordsScale = tc.height_scale / tc.width_scale / tc.grid_scale;
  vec3 normalDir = GetTexGradient(uHeightmapTexture, heightmapCoords, coordsScale, 2.0f);
  normalDir = mat3(uModelMatrix) * normalDir;

  vec3 lightDir = normalize(uLightDir);
  vec3 nNormalDir = normalize(normalDir);

  //lighting variables
  float diffuseFactor = dot(nNormalDir, lightDir);
  vec3 diffuseColor = max(diffuseFactor, 0.0) *
                      uLightColor;
  vec3 reflectDir = normalize(reflect(-lightDir, nNormalDir));
  vec3 viewDir = normalize(worldPos - uCameraPos);

  float specularFactor = max(dot(reflectDir, -viewDir), 0.0);
  float shininess = material.shininess / uShininessScale;
  specularFactor = pow(specularFactor, uSpecularPower) * shininess;
  //apply texture scaling/displacement
  vec2 transformedCoords = ApplyTexTileConfig(texCoords, tc, uNoiseTexture);
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  vec4 color = vec4(0.0f);
  float normalized_height = worldPos.y / tc.height_scale;
  float kHighThreshold = 0.7;
  float kMediumThreshold = 0.3;
  float kLowThreshold = 0.15;
  
  vec3 blending = max(abs(nNormalDir), 0.00001);
  float b = (blending.x + blending.y + blending.z);
  blending /= vec3(b, b, b);

  float scale = 0.03f;
  float frac = 0.0f;
  float blendFrom = 0.0f;

  vec2 xCoords = worldPos.yz * scale;
  vec2 yCoords = worldPos.xz * scale;
  vec2 zCoords = worldPos.xy * scale;
  
  //biome blending
  if (normalized_height > kHighThreshold) {
    frac = (normalized_height - kHighThreshold) / (1.0 - kHighThreshold);
    blendFrom = 0.0;
  } else if (normalized_height > kMediumThreshold) {
    frac = (normalized_height - kMediumThreshold) / (kHighThreshold - kMediumThreshold);
    blendFrom = 1.0f;
  } else if (normalized_height > kLowThreshold) {
    frac = (normalized_height - kLowThreshold) / (kMediumThreshold - kLowThreshold);
    blendFrom = 2.0f;
  } else {
    frac = 1.0f;
    blendFrom = 3.0f;
  }

  // tri planar mapping
  vec4 xaxis = mix(texture( uBlendTexture, vec3(xCoords, blendFrom + 1.0f)),
                   texture( uBlendTexture, vec3(xCoords, blendFrom)),
                   frac);
  vec4 yaxis = mix(texture( uBlendTexture, vec3(yCoords, blendFrom + 1.0f)),
                   texture( uBlendTexture, vec3(yCoords, blendFrom)),
                   frac);
  vec4 zaxis = mix(texture( uBlendTexture, vec3(zCoords, blendFrom + 1.0f)),
                   texture( uBlendTexture, vec3(zCoords, blendFrom)),
                   frac);

  color = xaxis * blending.x +
          yaxis * blending.y +
          zaxis * blending.z;
  
  //apply texture color variation
  color = TransformTexColor(color, texCoords, tc, uNoiseTexture);
  
  //apply lighting
  vec3 ambientColor = uAmbientLightColor * material.ambientColor * color.xyz;
  vec3 litColor = diffuseColor * color.xyz +
                  specularFactor * uLightColor * material.specularColor;
  
  //apply shadows
  if (diffuseFactor > 0.0f) {
    float bias = mix(tc.parallel_bias, tc.flat_bias, diffuseFactor) / tc.grid_scale;
    float shadowFactor = CalcShadowFactor(uDepthTexture ,lightSpacePosition, bias);
    litColor *= shadowFactor;
  }
  color = vec4(ambientColor + litColor, 1.0f);
  FragColor = color;
};
