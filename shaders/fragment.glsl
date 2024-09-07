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
uniform sampler2D uBlendTexture;

uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uCameraPos;
uniform float uSpecularPower;
uniform float uShininessScale;
uniform float uRepeatScale;
uniform float uRotationScale;
uniform float uTranslationScale;
uniform float uNoiseScale;

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

float FakeRandom(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float SampleNoise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    float a = FakeRandom(i);
    float b = FakeRandom(i + vec2(1.0, 0.0));
    float c = FakeRandom(i + vec2(0.0, 1.0));
    float d = FakeRandom(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}



vec4 BlendedTexture(vec2 texCoords) {
  vec2 scaledCoords = texCoords * uRepeatScale;
  vec2 tileCoords = floor(scaledCoords);
  vec2 localCoords = scaledCoords - tileCoords;
  float angle = SampleNoise(tileCoords*uNoiseScale) * uRotationScale;
  float translationX = SampleNoise(tileCoords*uNoiseScale*2.0f) * uTranslationScale;
  float translationY = SampleNoise(tileCoords*uNoiseScale*3.0f) * uTranslationScale;
  mat2 rotation = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
  vec2 rotatedCoords = rotation * (localCoords - 0.5) + 0.5;
  vec2 finalCoords = rotatedCoords + vec2(translationX, translationY); 
  vec4 textureColor = texture(uDiffuseTexture, finalCoords);
  return textureColor;
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
  
  // FragColor = texture(uDiffuseTexture, texCoords);
  FragColor = BlendedTexture(texCoords);
};
