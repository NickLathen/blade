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
  vec4 color;
  if (textureIdx >= 0) {
    vec4 materialColor = texture(uTextures[textureIdx % NUM_TEXTURES], texCoords);
    vec3 ambientColor = uAmbientLightColor * vec3(.4,.4,.4) * materialColor.rgb;
    vec3 litColor = diffuseColor * materialColor.rgb +
                    specularFactor * uLightColor * vec3(.5,.5,.5);
    color = vec4(ambientColor + litColor, .5);
  } else {
    vec3 materialColor = material.diffuseColor;
    vec3 ambientColor = uAmbientLightColor * material.ambientColor * materialColor;
    vec3 litColor = diffuseColor * materialColor +
                    specularFactor * uLightColor * material.specularColor;
    if (diffuseFactor > 0.0f) {
      float bias = mix(0.0000001, 0.000001, diffuseFactor);
      float shadowFactor = CalcShadowFactor(uDepthTexture, lightSpacePosition, bias);
      litColor *= shadowFactor;
    }
    color = vec4(ambientColor + litColor, 1.0f);
  }
  FragColor = color;
};
