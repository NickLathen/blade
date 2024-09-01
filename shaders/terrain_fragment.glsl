#version 300 es
precision highp float;

in vec3 normalDir;
in vec3 worldPos;
in vec2 texCoords;
in vec4 lightSpacePosition;
out vec4 FragColor;

uniform sampler2D uLightDepthTexture;
uniform sampler2D uDiffuseTexture;

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

float CalcShadowFactor(vec4 position) {
  vec3 ProjCoords = position.xyz / position.w;
  vec2 UVCoords;
  UVCoords.x = 0.5 * ProjCoords.x + 0.5;
  UVCoords.y = 0.5 * ProjCoords.y + 0.5;
  float z = 0.5 * ProjCoords.z + 0.5;
  if (z < 0.0) {
    return 0.5;
  }
  float Depth = texture(uLightDepthTexture, UVCoords).x;
  float bias = .00025;
  if (Depth + bias < z) {
    return 0.5;
  }
  return 1.0;
}

void main() {
  Material material = uMaterial.materials[0];
  vec3 nNormalDir = normalize(normalDir);
  vec3 lightDir = normalize(uLightDir);
  vec3 lightRelativePosition = uLightPos - worldPos;
  vec3 specLightDir = normalize(lightRelativePosition);

  //diffuse lighting
  vec3 diffuseColor = max(dot(nNormalDir, specLightDir), 0.0) *
                      uLightColor;

  //specular lighting
  vec3 reflectDir = normalize(reflect(-specLightDir, nNormalDir));
  vec3 viewDir = normalize(worldPos - uCameraPos);
  float shininess = material.shininess / uShininessScale;
  float specularFactor = max(dot(reflectDir, -viewDir), 0.0);
  specularFactor = pow(specularFactor, uSpecularPower) * shininess;

  vec3 litColor = diffuseColor * material.diffuseColor +
                  specularFactor * uLightColor * material.specularColor;
    
  vec3 ambientColor = uAmbientLightColor * material.ambientColor * material.diffuseColor;
  vec3 finalColor = ambientColor +
                    CalcShadowFactor(lightSpacePosition) * litColor;
  FragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0f);
};
