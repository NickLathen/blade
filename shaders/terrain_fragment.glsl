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
uniform sampler2D uHeightmapTexture;

uniform mat4 uModelMatrix;
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

struct TileConfig {
  float height_scale;
  float width_scale;
  float grid_scale;
  int resolution;
  float repeat_scale;
  float rotation_scale;
  float translation_scale;
  float noise_scale;
  float hue_scale;
  float saturation_scale;
  float brightness_scale;
};
layout(std140) uniform uTileConfigBlock {
  TileConfig tileConfig;
} uTileConfig;

float CalcShadowFactor(vec4 position, float diffuseFactor) {
  vec3 ProjCoords = position.xyz / position.w;
  vec3 UVCoords;
  UVCoords.x = 0.5 * ProjCoords.x + 0.5;
  UVCoords.y = 0.5 * ProjCoords.y + 0.5;
  UVCoords.z = 0.5 * ProjCoords.z + 0.5;
  if (UVCoords.z < 0.0 || UVCoords.x < 0.0 || UVCoords.x > 1.0 || UVCoords.y < 0.0 || UVCoords.y > 1.0) {
    return 1.0;
  }
  float bias = mix(0.0005, 0.002, clamp(abs(diffuseFactor), 0.0, 1.0));
  UVCoords.z -= bias;
  float shadowFactor = 0.0;
  float texelSize = 1.0 / float(textureSize(uDepthTexture, 0));
  for (int y = -1 ; y <= 1 ; y++) {
    for (int x = -1 ; x <= 1 ; x++) {
      vec3 offset = vec3(float(x) * texelSize,
                          float(y) * texelSize,
                          0.0f);
      shadowFactor += texture(uDepthTexture, UVCoords + offset);
    }
  }
  return (0.5 + (shadowFactor / 18.0));
}

float SampleNoise(vec2 st) {
  return texture(uNoiseTexture, st).x;
}

vec2 ScaleToCenter(vec2 coords, float scale) {
  return coords + (0.5f - coords) * scale;
}

vec2 TransformTexCoords(vec2 texCoords, TileConfig tc) {
  vec2 scaledCoords = texCoords * tc.repeat_scale;
  vec2 tileCoords = floor(scaledCoords);
  vec2 noiseCoords = tileCoords / tc.repeat_scale;
  noiseCoords = ScaleToCenter(noiseCoords, 0.5f);
  vec2 localCoords = scaledCoords - tileCoords;
  float angle = SampleNoise((noiseCoords - 0.2f) * tc.noise_scale) * tc.rotation_scale;
  float translationX = SampleNoise(noiseCoords * tc.noise_scale) * tc.translation_scale;
  float translationY = SampleNoise((noiseCoords + 0.2f) * tc.noise_scale) * tc.translation_scale;
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

vec4 TransformTexColor(vec4 color, vec2 texCoords, TileConfig tc) {
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  vec4 colorOut = color;
  float rHue = SampleNoise(noiseCoords + 0.1f);
  //saturation -1.0 to 0.0f (desaturate only)
  float rSaturation = -SampleNoise(noiseCoords - 0.1f);
  //brightness -.5 to +.5
  float rBrightness = SampleNoise(noiseCoords + 0.2f) - 0.5f;
  colorOut = AdjustHue(colorOut, tc.hue_scale * rHue);
  colorOut = AdjustSaturation(colorOut, clamp(1.0f + tc.saturation_scale * rSaturation, 0.0, 1.0));
  colorOut = AdjustBrightness(colorOut, 1.0f + tc.brightness_scale * rBrightness);
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
  TileConfig tc = uTileConfig.tileConfig;
  Material material = uMaterial.materials[materialIdx];

  vec3 normalDir = GetGradient(uHeightmapTexture, terrainCoords, tc.height_scale, tc.width_scale, tc.grid_scale);
  normalDir = mat3(uModelMatrix) * normalDir;

  vec3 lightDir = normalize(uLightDir);
  vec3 nNormalDir = normalize(normalDir);

  //lighting variables
  float diffuseFactor = dot(nNormalDir, lightDir);
  vec3 diffuseColor = max(diffuseFactor, 0.0) *
                      uLightColor;
  vec3 reflectDir = normalize(reflect(-lightDir, nNormalDir));
  vec3 viewDir = normalize(worldPos - uCameraPos);
  float shininess = material.shininess / uShininessScale;
  float specularFactor = max(dot(reflectDir, -viewDir), 0.0);
  specularFactor = pow(specularFactor, uSpecularPower) * shininess;
  
  //apply texture scaling/displacement
  vec2 transformedCoords = TransformTexCoords(texCoords, tc);
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  //apply texture blending
  vec4 color = mix(texture(uDiffuseTexture, transformedCoords),
                   texture(uBlendTexture, transformedCoords),
                   SampleNoise(noiseCoords - 0.25f)
                  );
  //apply texture color variation
  color = TransformTexColor(color, texCoords, tc);
  //apply lighting
  vec3 ambientColor = uAmbientLightColor * material.ambientColor * color.xyz;
  vec3 litColor = diffuseColor * color.xyz +
                  specularFactor * uLightColor * material.specularColor;
  //apply shadows
  if (diffuseFactor > 0.0f) {
    float shadowFactor = CalcShadowFactor(lightSpacePosition, abs(diffuseFactor));
    litColor *= shadowFactor;
  }
  color = vec4(ambientColor + litColor, 1.0f);
  FragColor = color;
};
