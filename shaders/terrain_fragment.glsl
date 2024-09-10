#version 300 es
precision highp float;

in vec3 worldPos;
in vec2 texCoords;
in vec2 heightmapCoords;
in vec4 lightSpacePosition;
flat in uint materialIdx;
out vec4 FragColor;

uniform sampler2DShadow uDepthTexture;
uniform sampler2D uDiffuseTexture;
uniform sampler2D uNoiseTexture;
uniform sampler2D uHeightmapTexture;
uniform sampler2DArray uBlendTexture;

uniform mat4 uModelMatrix;
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
  float flat_bias;
  float parallel_bias;
};
layout(std140) uniform uTileConfigBlock {
  TileConfig tileConfig;
} uTileConfig;

float CalcShadowFactor(vec4 position, float bias) {
  float kShadowStrength = 0.8;
  vec3 ProjCoords = position.xyz / position.w;
  vec3 UVCoords;
  UVCoords.x = 0.5 * ProjCoords.x + 0.5;
  UVCoords.y = 0.5 * ProjCoords.y + 0.5;
  UVCoords.z = 0.5 * ProjCoords.z + 0.5;
  if (UVCoords.z < 0.0 || UVCoords.x < 0.0 || UVCoords.x > 1.0 || UVCoords.y < 0.0 || UVCoords.y > 1.0) {
    return 1.0;
  }
  UVCoords.z -= bias;
  float shadowFactor = 0.0;
  float texelSize = 1.0 / float(textureSize(uDepthTexture, 0));
  int nNeighbors = 2;
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

vec2 ScaleToCenter(vec2 coords, float scale) {
  return coords + (0.5f - coords) * scale;
}

vec2 ApplyTexTileConfig(vec2 texCoords, TileConfig tc, sampler2D noiseTexture) {
  vec2 scaledCoords = texCoords * tc.repeat_scale;
  vec2 tileCoords = floor(scaledCoords);
  vec2 noiseCoords = tileCoords / tc.repeat_scale;
  noiseCoords = ScaleToCenter(noiseCoords, 0.5f);
  vec2 angleNoiseCoords = (noiseCoords - 0.2f) * tc.noise_scale;
  vec2 xNoiseCoords = noiseCoords * tc.noise_scale;
  vec2 yNoiseCoords = (noiseCoords + 0.2f) * tc.noise_scale;
  float angle =        texture(noiseTexture, angleNoiseCoords) * tc.rotation_scale;
  float translationX = texture(noiseTexture, xNoiseCoords)     * tc.translation_scale;
  float translationY = texture(noiseTexture, yNoiseCoords)     * tc.translation_scale;
  mat2 rotation = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
  vec2 localCoords = scaledCoords - tileCoords;
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

vec4 TransformTexColor(vec4 color, vec2 texCoords, TileConfig tc, sampler2D noiseTexture) {
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  vec4 colorOut = color;
  float rHue = texture(noiseTexture, (noiseCoords + 0.1f));
  //saturation -1.0 to 0.0f (desaturate only)
  float rSaturation = -texture(noiseTexture, (noiseCoords - 0.1f));
  //brightness -.5 to +.5
  float rBrightness = texture(noiseTexture, (noiseCoords + 0.2f) - 0.5f);
  colorOut = AdjustHue(colorOut, tc.hue_scale * rHue);
  colorOut = AdjustSaturation(colorOut, clamp(1.0f + tc.saturation_scale * rSaturation, 0.0, 1.0));
  colorOut = AdjustBrightness(colorOut, 1.0f + tc.brightness_scale * rBrightness);
  return colorOut;
}

//coordsScale tells us how to scale texture coordinates to world coordinates
vec3 GetTexGradient(sampler2D tex, vec2 coords, float coordsScale, float sampleWidth) {
  float texelSize = 1.0 / float(textureSize(tex, 0));
  float epsilon = sampleWidth * texelSize;
  float gradScale = coordsScale / (epsilon * 2.0f);
  float heightRight = texture(tex, coords + vec2(epsilon, 0.0)).r;
  float heightLeft  = texture(tex, coords + vec2(-epsilon, 0.0)).r;
  float heightUp    = texture(tex, coords + vec2(0.0, epsilon)).r;
  float heightDown  = texture(tex, coords + vec2(0.0, -epsilon)).r;
  float dy_dx = -(heightRight - heightLeft) * gradScale;
  float dy_dz = (heightUp - heightDown) * gradScale;
  return normalize(vec3(dy_dx, 1.0f, dy_dz));
}

void main() {
  TileConfig tc = uTileConfig.tileConfig;
  Material material = uMaterial.materials[materialIdx];

  float coordsScale = tc.height_scale / tc.width_scale / tc.grid_scale;
  vec3 normalDir = GetTexGradient(uHeightmapTexture, heightmapCoords, coordsScale, 10.0f);
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
  vec2 transformedCoords = ApplyTexTileConfig(texCoords, tc, uNoiseTexture);
  vec2 noiseCoords = ScaleToCenter(texCoords, 0.5f);
  vec4 color = vec4(0.0f);
  float normalized_height = worldPos.y / tc.height_scale;
  float kHighThreshold = 0.7;
  float kMediumThreshold = 0.3;
  float kLowThreshold = 0.15;
  
  if (normalized_height > kHighThreshold) {
    float frac = (normalized_height - kHighThreshold) / (1.0 - kHighThreshold);
    color = mix(texture(uBlendTexture, vec3(transformedCoords, 1.0)),
                texture(uBlendTexture, vec3(transformedCoords, 0.0)),
                frac);
  } else if (normalized_height > kMediumThreshold) {
    float frac = (normalized_height - kMediumThreshold) / (kHighThreshold - kMediumThreshold);
    color = mix(texture(uBlendTexture, vec3(transformedCoords, 2.0)),
                texture(uBlendTexture, vec3(transformedCoords, 1.0)),
                frac);
  } else if (normalized_height > kLowThreshold) {
    float frac = (normalized_height - kLowThreshold) / (kMediumThreshold - kLowThreshold);
    color = mix(texture(uBlendTexture, vec3(transformedCoords, 3.0)),
                texture(uBlendTexture, vec3(transformedCoords, 2.0)),
                frac);
  } else {
    color = texture(uBlendTexture, vec3(transformedCoords, 3.0));
  }
  //apply texture color variation
  color = TransformTexColor(color, texCoords, tc, uNoiseTexture);
  //apply lighting
  vec3 ambientColor = uAmbientLightColor * material.ambientColor * color.xyz;
  vec3 litColor = diffuseColor * color.xyz +
                  specularFactor * uLightColor * material.specularColor;
  //apply shadows
  if (diffuseFactor > 0.0f) {
    float bias = mix(tc.parallel_bias, tc.flat_bias, diffuseFactor) / tc.grid_scale;
    float shadowFactor = CalcShadowFactor(lightSpacePosition, bias);
    litColor *= shadowFactor;
  }
  color = vec4(ambientColor + litColor, 1.0f);
  FragColor = color;
};
