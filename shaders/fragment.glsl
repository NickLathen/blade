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
uniform sampler2D uNoiseTexture;

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
uniform float uSaturation;
uniform float uHueOffset;
uniform float uBrightness;

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

float SampleNoise(vec2 st) {
  return texture(uNoiseTexture, st).x;
}

vec2 TransformTexCoords(vec2 texCoords) {
  vec2 scaledCoords = texCoords * uRepeatScale;
  vec2 tileCoords = floor(scaledCoords);
  vec2 noiseCoords = tileCoords / uRepeatScale;
  vec2 localCoords = scaledCoords - tileCoords;
  float angle = SampleNoise(noiseCoords * uNoiseScale) * uRotationScale;
  float translationX = SampleNoise(noiseCoords * uNoiseScale) * uTranslationScale;
  float translationY = SampleNoise(noiseCoords * uNoiseScale) * uTranslationScale;
  mat2 rotation = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
  vec2 rotatedCoords = rotation * (localCoords - 0.5) + 0.5;
  vec2 finalCoords = rotatedCoords + vec2(translationX, translationY);
  return finalCoords;
}

vec4 AdjustSaturation(vec4 color, float saturation) {
  float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  vec3 gray = vec3(luminance);
  vec3 adjustedColor = mix(gray, color.rgb, saturation);
  return vec4(adjustedColor, color.a);
}

vec3 RgbToHsv(vec3 c) {
  vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
  float d = q.x - min(q.w, q.y);
  float e = 1.0e-10;
  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 HsvToRgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 AdjustHue(vec4 color, float hueOffset) {
  vec3 hsv = RgbToHsv(color.rgb);
  hsv.x = fract(hsv.x + hueOffset);
  vec3 adjustedColor = HsvToRgb(hsv);
  return vec4(adjustedColor, color.a);
}

vec4 AdjustBrightness(vec4 color, float brightness) {
  vec3 adjustedColor = brightness * color.xyz;
  return vec4(clamp(adjustedColor, 0.0, 1.0), color.a);
}

vec4 TransformTexColor(vec4 color, vec2 texCoords) {
  vec2 noiseCoords = texCoords + ((0.5f - texCoords) * .5);
  vec4 colorOut = color;
  float rHue = SampleNoise(noiseCoords + 0.1f);
  float rSaturation = SampleNoise(noiseCoords - 0.1f) - 0.5;
  float rBrightness = SampleNoise(noiseCoords + 0.2f) - 0.5;
  colorOut = AdjustHue(colorOut, uHueOffset * rHue);
  colorOut = AdjustSaturation(colorOut, 1.0f + uSaturation * rSaturation);
  colorOut = AdjustBrightness(colorOut, 1.0f + uBrightness * rBrightness);
  return colorOut;
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
  
  vec2 transformedCoords = TransformTexCoords(texCoords);
  vec4 color = texture(uDiffuseTexture, transformedCoords);
  color = TransformTexColor(color, texCoords);
  FragColor = color * CalcShadowFactor(lightSpacePosition, diffuseFactor);
};
