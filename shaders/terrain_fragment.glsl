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
  vec3 normalDir = GetTexGradient(uHeightmapTexture, heightmapCoords, coordsScale, 0.5f);
  normalDir = mat3(uModelMatrix) * normalDir;

  vec3 lightDir = normalize(uLightDir);
  vec3 nNormalDir = normalize(normalDir);

  //lighting variables
  float diffuseFactor = dot(nNormalDir, lightDir);
  vec3 diffuseColor = max(diffuseFactor, 0.0) *
                      uLightColor;
  vec3 reflectDir = normalize(reflect(-lightDir, nNormalDir));
  vec3 viewDir = normalize(worldPos - uCameraPos);
  float shininess = material.shininess / uShininessScale;
  float specularFactor = max(dot(reflectDir, -viewDir), 0.0);
  specularFactor = pow(specularFactor, uSpecularPower) * shininess;
  
  //apply texture scaling/displacement
  vec2 transformedCoords = ApplyTexTileConfig(texCoords, tc, uNoiseTexture);
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  vec4 color = vec4(0.0f);
  float normalized_height = worldPos.y / tc.height_scale;
  float kHighThreshold = 0.7;
  float kMediumThreshold = 0.3;
  float kLowThreshold = 0.15;
  
  if (normalized_height > kHighThreshold) {
    float frac = (normalized_height - kHighThreshold) / (1.0 - kHighThreshold);
    color = mix(texture(uBlendTexture, vec3(transformedCoords, 1.0)),
                texture(uBlendTexture, vec3(transformedCoords, 0.0)),
                frac);
  } else if (normalized_height > kMediumThreshold) {
    float frac = (normalized_height - kMediumThreshold) / (kHighThreshold - kMediumThreshold);
    color = mix(texture(uBlendTexture, vec3(transformedCoords, 2.0)),
                texture(uBlendTexture, vec3(transformedCoords, 1.0)),
                frac);
  } else if (normalized_height > kLowThreshold) {
    float frac = (normalized_height - kLowThreshold) / (kMediumThreshold - kLowThreshold);
    color = mix(texture(uBlendTexture, vec3(transformedCoords, 3.0)),
                texture(uBlendTexture, vec3(transformedCoords, 2.0)),
                frac);
  } else {
    color = texture(uBlendTexture, vec3(transformedCoords, 3.0));
  }
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
