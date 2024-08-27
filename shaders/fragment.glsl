#version 300 es
precision highp float;

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

in vec3 normalDir;
in vec3 worldPos;
flat in uint materialIdx;
out vec4 FragColor;
void main() {
  Material material = uMaterial.materials[materialIdx];
  vec3 ambientColor = uAmbientLightColor;
  vec3 nNormalDir = normalize(normalDir);
  vec3 lightDir = normalize(uLightDir);
  vec3 specLightDir = normalize(uLightPos - worldPos);

  //diffuse color
  vec3 diffuseColor = max(dot(nNormalDir, specLightDir), 0.0) *
                      uLightColor;

  //specular
  vec3 reflectDir = normalize(reflect(-specLightDir, nNormalDir));
  vec3 viewDir = normalize(worldPos - uCameraPos);

  float shininess = material.shininess / uShininessScale;
  float specularFactor = max(dot(reflectDir, -viewDir), 0.0);
  specularFactor = pow(specularFactor, uSpecularPower) * shininess;

  vec3 finalColor = ambientColor * material.ambientColor * material.diffuseColor * 0.0f +
                    diffuseColor * material.diffuseColor +
                    specularFactor * uLightColor * material.specularColor;
  FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0f);
};
