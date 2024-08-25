#version 300 es
precision highp float;

uniform vec3 uCameraPos;
uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;

layout(std140) uniform uMaterialBlock {
  vec3 ambientColor;
  vec3 diffuseColor;
  vec3 specularColor;
  float shininess;
} uMaterial;

in vec3 normalDir;
in vec3 worldPos;
out vec4 FragColor;
void main() {
  vec3 viewDir = normalize(worldPos - uCameraPos);
  vec3 lightDir = normalize(uLightDir);
  vec3 reflectDir = normalize(reflect(lightDir, normalDir));

  //Calculate Light Colors
  vec3 ambientColor = uAmbientLightColor;
  vec3 diffuseColor = max(dot(normalDir, -lightDir), 0.0) *
                       uLightColor;
  vec3 specularColor = max(dot(reflectDir, viewDir), 0.0) *
                        uLightColor;
  float specularFactor = uMaterial.shininess / 500.0f;
  vec3 finalColor = clamp (
                      ambientColor * uMaterial.ambientColor * uMaterial.diffuseColor +
                      diffuseColor  * uMaterial.diffuseColor +
                      specularColor * uMaterial.specularColor * specularFactor
                      ,0.0f, 1.0f);
  FragColor = vec4(finalColor, 1.0f);
};
