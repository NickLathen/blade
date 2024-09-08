#version 300 es
precision highp float;

in vec3 worldPos;
in vec2 texCoords;
in vec2 terrainCoords;
in vec4 lightSpacePosition;
flat in uint materialIdx;
out vec4 FragColor;

uniform sampler2DShadow uDepthTexture;
uniform sampler2D uDiffuseTexture;
uniform sampler2D uBlendTexture;
uniform sampler2D uNoiseTexture;


uniform mat4 uModelMatrix;
uniform vec3 uAmbientLightColor;
uniform vec3 uLightDir;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uCameraPos;
uniform float uSpecularPower;
uniform float uShininessScale;
uniform float uHeightScale;
uniform float uWidthScale;
uniform float uGridScale;
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

vec2 ScaleToCenter(vec2 coords, float scale) {
  return coords + (0.5f - coords) * scale;
}

vec2 TransformTexCoords(vec2 texCoords) {
  vec2 scaledCoords = texCoords * uRepeatScale;
  vec2 tileCoords = floor(scaledCoords);
  vec2 noiseCoords = tileCoords / uRepeatScale;
  noiseCoords = ScaleToCenter(noiseCoords, 0.5f);
  vec2 localCoords = scaledCoords - tileCoords;
  float angle = SampleNoise((noiseCoords - 0.2f) * uNoiseScale) * uRotationScale;
  float translationX = SampleNoise(noiseCoords * uNoiseScale) * uTranslationScale;
  float translationY = SampleNoise((noiseCoords + 0.2f) * uNoiseScale) * uTranslationScale;
  mat2 rotation = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
  vec2 rotatedCoords = rotation * (localCoords - 0.5) + 0.5;
  vec2 finalCoords = rotatedCoords + vec2(translationX, translationY);
  return finalCoords;
}

vec4 AdjustSaturation(vec4 color, float saturation) {
  float luminance = abs(dot(color.rgb, vec3(0.299, 0.587, 0.114)));
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
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  vec4 colorOut = color;
  float rHue = SampleNoise(noiseCoords + 0.1f);
  //saturation -1.0 to 0.0f (desaturate only)
  float rSaturation = -SampleNoise(noiseCoords - 0.1f);
  //brightness -.5 to +.5
  float rBrightness = SampleNoise(noiseCoords + 0.2f) - 0.5f;
  colorOut = AdjustHue(colorOut, uHueOffset * rHue);
  colorOut = AdjustSaturation(colorOut, clamp(1.0f + uSaturation * rSaturation, 0.0, 1.0));
  colorOut = AdjustBrightness(colorOut, 1.0f + uBrightness * rBrightness);
  return colorOut;
}

vec3 GetGradient(sampler2D tex, vec2 coords, float heightScale, float widthScale, float gridScale) {
  float epsilon = 0.001f;
  float heightCenter = texture(tex, terrainCoords).r * heightScale;
  float heightRight  = texture(tex, terrainCoords + vec2(epsilon, 0.0)).r * heightScale;
  float heightUp     = texture(tex, terrainCoords + vec2(0.0, epsilon)).r * heightScale;
  float dy_dx = (heightRight - heightCenter) * widthScale / (epsilon * gridScale);
  float dy_dz = (heightUp - heightCenter) * widthScale / (epsilon * gridScale);
  return normalize(vec3(dy_dx, 1.0f, dy_dz));
}

void main() {
  Material material = uMaterial.materials[materialIdx];

  vec3 normalDir = GetGradient(uNoiseTexture, terrainCoords, uHeightScale, uWidthScale, uGridScale);
  normalDir = mat3(uModelMatrix) * normalDir;

  vec3 lightDir = normalize(uLightDir);
  vec3 nNormalDir = normalize(normalDir);

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
  
  vec2 transformedCoords = TransformTexCoords(texCoords);
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  vec4 color = mix(texture(uDiffuseTexture, transformedCoords),
                   texture(uBlendTexture, transformedCoords),
                   SampleNoise(noiseCoords - 0.25f)
                  );
  color = TransformTexColor(color, texCoords);
  vec3 litColor = diffuseColor * color.xyz +
                  specularFactor * uLightColor * material.specularColor;
  vec3 ambientColor = .1 * color.xyz;
  color = vec4(ambientColor + litColor, 1.0f);
  FragColor = color;
};
