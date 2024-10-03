#version 300 es
precision highp float;

#include "structs.glsl"
#include "functions.glsl"

#define NUM_TEXTURES 32
uniform sampler2D uTextures[NUM_TEXTURES];

//Materials UBO
#define NUM_MATERIALS 256
layout(std140) uniform uMaterialBlock {
  Material materials[NUM_MATERIALS];
} uMaterial;

in vec3 normalDir;
in vec3 worldPos;
in vec2 texCoords;
in vec4 lightSpacePosition;
flat in uint materialIdx;
flat in int textureIdx;
out vec4 FragColor;

uniform sampler2DShadow uDepthTexture;

uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uCameraPos;
uniform float uSpecularPower;
uniform float uShininessScale;

void main() {

  Material material = uMaterial.materials[materialIdx];
  vec3 nNormalDir = normalize(normalDir);
  vec3 lightDir = normalize(uLightDir);

  //diffuse lighting
  float diffuseFactor = dot(nNormalDir, lightDir);
  vec3 diffuseColor = max(diffuseFactor, 0.0) *
                      uLightColor;

  //specular lighting
  vec3 reflectDir = normalize(reflect(-lightDir, nNormalDir));
  vec3 viewDir = normalize(worldPos - uCameraPos);
  float shininess = material.shininess / uShininessScale;
  float specularFactor = max(dot(reflectDir, -viewDir), 0.0);
  specularFactor = pow(specularFactor, uSpecularPower) * shininess;
  vec3 ambientColor, litColor;
  float alpha = 1.0;
  if (textureIdx >= 0) {
    vec4 color = texture(uTextures[textureIdx % NUM_TEXTURES], mod(texCoords, 1.0));
    ambientColor = uAmbientLightColor * color.rgb;
    litColor = diffuseColor * color.rgb +
               specularFactor * uLightColor;
    alpha = color.a;
  } else {
    vec3 color = material.diffuseColor;
    ambientColor = uAmbientLightColor * material.ambientColor * color;
    litColor = diffuseColor * color +
               specularFactor * uLightColor * material.specularColor;
  }
  if (diffuseFactor > 0.0f) {
    float bias = mix(0.0002, 0.002, diffuseFactor);
    float shadowFactor = CalcShadowFactor(uDepthTexture, lightSpacePosition, bias);
    litColor *= shadowFactor;
  }
  if (alpha < 0.5) {
    FragColor = vec4(1,1,1,1);
    return;
  }
  FragColor = vec4(ambientColor + litColor, alpha);
};
