#version 300 es
precision highp float;

in vec3 normalDir;
in vec3 worldPos;
in vec2 texCoords;
in vec4 lightSpacePosition;
flat in uint materialIdx;
out vec4 FragColor;

uniform sampler2DShadow uDepthTexture;
uniform sampler2D uDiffuseTexture;
uniform sampler2D uNoiseTexture;

uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uCameraPos;
uniform float uSpecularPower;
uniform float uShininessScale;

//Materials UBO
#define NUM_MATERIALS 256
struct Material {
  vec3 ambientColor;
  vec3 diffuseColor;
  vec3 specularColor;
  float shininess;
};
layout(std140) uniform uMaterialBlock {
  Material materials[NUM_MATERIALS];
} uMaterial;

float CalcShadowFactor(vec4 position, float diffuseFactor) {
  float kShadowStrength = 0.8;
  vec3 ProjCoords = position.xyz / position.w;
  vec3 UVCoords;
  UVCoords.x = 0.5 * ProjCoords.x + 0.5;
  UVCoords.y = 0.5 * ProjCoords.y + 0.5;
  UVCoords.z = 0.5 * ProjCoords.z + 0.5;
  if (UVCoords.z < 0.0 || UVCoords.x < 0.0 || UVCoords.x > 1.0 || UVCoords.y < 0.0 || UVCoords.y > 1.0) {
    return 1.0;
  }
  float bias = mix(0.0000001, 0.000001, diffuseFactor);
  UVCoords.z -= bias;
  float shadowFactor = 0.0;
  float texelSize = 1.0 / float(textureSize(uDepthTexture, 0));
  int nNeighbors = 1;
  float kernelSize = pow(float(nNeighbors) * 2.0 + 1.0, 2.0f);
  for (int y = -nNeighbors ; y <= nNeighbors ; y++) {
    for (int x = -nNeighbors ; x <= nNeighbors ; x++) {
      vec3 offset = vec3(float(x) * texelSize,
                          float(y) * texelSize,
                          0.0f);
      shadowFactor += texture(uDepthTexture, UVCoords + offset);
    }
  }
  return ((1.0 - kShadowStrength) + (shadowFactor * kShadowStrength) / kernelSize);
}

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
  
  vec3 materialColor = material.diffuseColor;
  vec3 ambientColor = uAmbientLightColor * material.ambientColor * materialColor;
  vec3 litColor = diffuseColor * materialColor +
                  specularFactor * uLightColor * material.specularColor;
  if (diffuseFactor > 0.0f) {
    float shadowFactor = CalcShadowFactor(lightSpacePosition, diffuseFactor);
    litColor *= shadowFactor;
  }
  vec4 color = vec4(ambientColor + litColor, 1.0f);
  FragColor = color;
};
