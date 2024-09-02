#version 300 es
precision highp float;

in vec3 normalDir;
in vec3 worldPos;
in vec2 texCoords;
in vec4 lightSpacePosition;
flat in uint materialIdx;
out vec4 FragColor;

uniform sampler2DShadow uDepthTexture;

uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightPos;
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
  vec3 ProjCoords = position.xyz / position.w;
  vec3 UVCoords;
  UVCoords.x = 0.5 * ProjCoords.x + 0.5;
  UVCoords.y = 0.5 * ProjCoords.y + 0.5;
  UVCoords.z = 0.5 * ProjCoords.z + 0.5;
  if (UVCoords.z < 0.0 || UVCoords.x < 0.0 || UVCoords.x > 1.0 || UVCoords.y < 0.0 || UVCoords.y > 1.0) {
    return 1.0;
  }
  float bias = mix(0.0001, 0.001, clamp(abs(diffuseFactor), 0.0, 1.0));
  UVCoords.z -= bias;
  float Depth = texture(uDepthTexture, UVCoords);
  return 0.5 + (Depth * 0.5f);
}

void main() {
  Material material = uMaterial.materials[materialIdx];
  vec3 nNormalDir = normalize(normalDir);
  vec3 lightDir = normalize(uLightDir);
  vec3 lightRelativePosition = uLightPos - worldPos;
  vec3 specLightDir = normalize(lightRelativePosition);

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

  vec3 litColor = diffuseColor * material.diffuseColor +
                  specularFactor * uLightColor * material.specularColor;
    
  vec3 ambientColor = uAmbientLightColor * material.ambientColor * material.diffuseColor;
  vec3 finalColor = ambientColor +
                    CalcShadowFactor(lightSpacePosition, diffuseFactor) * litColor;
  FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0f);
};
