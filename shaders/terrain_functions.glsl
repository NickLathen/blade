ivec2 GetHeightmapGridCoords(int resolution, int vertexId) {
  int gridWidth = resolution * 2 + 2;
  int vertX = vertexId % gridWidth;
  int vertY = vertexId / gridWidth;
  if (vertX < gridWidth - 2) {
    return ivec2(vertX / 2, vertY + 1 - vertX % 2);
  } else if (vertX == gridWidth - 2) {
    return ivec2(resolution - 1, vertY);
  } else if (vertX == gridWidth - 1) {
    return ivec2(0, vertY + 2);
  }
}

vec2 GetGridTexCoords(ivec2 gridCoords, int resolution) {
  float fResolution = float(resolution);
  vec2 halfTexel = vec2(0.5 / fResolution, 0.5 / fResolution);
  return (vec2(gridCoords) / fResolution) + halfTexel;
}

vec2 GetHeightmapTexCoords(TileConfig tc, int vertexId) {
  return GetGridTexCoords(
    GetHeightmapGridCoords(tc.resolution, vertexId),
    tc.resolution
  );
}

vec2 GetHeightmapCoords(TileConfig tc, vec2 texCoords) {
  return texCoords * tc.width_scale;
}

vec3 GetHeightmapPosition(TileConfig tc, sampler2D heightmap, vec2 texCoords, vec2 heightmapCoords) {
  float height = texture(heightmap, heightmapCoords).r;
  height *= tc.height_scale;
  return vec3(
    (texCoords.x - 0.5) * tc.grid_scale,
    height,
    -(texCoords.y - 0.5) * tc.grid_scale
  );
}

vec3 GetHeightmapSkirtPosition(TileConfig tc, sampler2D heightmap, int vertexId) {
  int skirtWidth = tc.resolution * 2;
  int endIndex = tc.resolution - 1;
  int vertX = vertexId % skirtWidth;
  int vertX2 = vertX / 2;
  int vertXmod2 = vertX % 2;
  int vertY = vertexId / skirtWidth;
  vec2 texCoords;
  if (vertY == 0) {
    // 0,0 to 1,0
    texCoords = GetGridTexCoords(ivec2(vertX2, 0), tc.resolution);
  } else if (vertY == 1) {
    // 1,0 to 1,1
    texCoords = GetGridTexCoords(ivec2(endIndex, vertX2), tc.resolution);
  } else if (vertY == 2) {
    // 1,1 to 0,1
    texCoords = GetGridTexCoords(ivec2(endIndex - vertX2, endIndex), tc.resolution);
  } else if (vertY == 3) {
    // 0,1 to 0,0
    texCoords = GetGridTexCoords(ivec2(0, endIndex - vertX2), tc.resolution);
  } else if (vertY == 4) {
    // skirt floor quad
    texCoords = GetGridTexCoords(ivec2(endIndex * vertX2, endIndex * vertXmod2), tc.resolution);
  }
  vec2 heightmapCoords = GetHeightmapCoords(tc, texCoords);
  vec3 terrainPosition = GetHeightmapPosition(tc, heightmap, texCoords, heightmapCoords);
  if (vertXmod2 == 1 || vertY == 4) {
    terrainPosition.y = -tc.height_scale;
  }
  return terrainPosition;
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
