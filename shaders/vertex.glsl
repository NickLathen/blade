#version 300 es

uniform mat4 uMVP;
uniform mat3 uWorldMatrix;
uniform vec3 uCameraPos;
uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;

uniform mediump uint uMaterialIdx;

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

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
out vec4 color;
void main() {
    vec3 worldPos = uWorldMatrix * aPos;
    vec3 normalDir = normalize(uWorldMatrix * aNormal);
    vec3 viewDir = normalize(worldPos - uCameraPos);
    vec3 lightDir = normalize(uLightDir);
    vec3 reflectDir = normalize(reflect(lightDir, normalDir));
    Material material = uMaterial.materials[uMaterialIdx];

    vec3 ambientColor = uAmbientLightColor;
    vec3 diffuseColor = max(dot(normalDir, -lightDir), 0.0) *
                        uLightColor;
    vec3 specularColor = max(dot(reflectDir, viewDir), 0.0) *
                            uLightColor;
    float specularFactor = material.shininess / 3000.0f;
    vec3 finalColor = clamp (
                      ambientColor * material.ambientColor * material.diffuseColor +
                      diffuseColor  * material.diffuseColor +
                      specularColor * material.specularColor * specularFactor
                      ,0.0f, 1.0f);
    color = vec4(finalColor, 1.0);

    gl_Position = uMVP * vec4(aPos, 1.0);
}
