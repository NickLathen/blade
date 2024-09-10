#version 300 es

uniform sampler2D uHeightmapTexture;

uniform mat4 uMVP;

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

vec2 GetGridTexCoords(ivec2 gridCoords, int resolution) {
  float fResolution = float(resolution);
  vec2 halfTexel = vec2(0.5 / fResolution, 0.5 / fResolution);
  return (vec2(gridCoords) / fResolution) + halfTexel;
}

vec2 GetHeightmapCoords(TileConfig tc, vec2 texCoords) {
  return texCoords * tc.width_scale;
}

vec3 GetTerrainPosition(TileConfig tc, vec2 texCoords, sampler2D heightmap, vec2 heightmapCoords) {
  float height = texture(heightmap, heightmapCoords).r;
  height *= tc.height_scale;
  return vec3(
    (texCoords.x - 0.5) * tc.grid_scale,
    height,
    -(texCoords.y - 0.5) * tc.grid_scale
  );
}

vec3 GetSkirtVertPosition(TileConfig tc, int vertexId, sampler2D heightmap) {
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
  vec3 terrainPosition = GetTerrainPosition(tc, texCoords, uHeightmapTexture, heightmapCoords);
  if (vertXmod2 == 1 || vertY == 4) {
    terrainPosition.y = -tc.height_scale;
  }
  return terrainPosition;
}

flat out vec4 color;
void main() {
  TileConfig tc = uTileConfig.tileConfig;
  vec3 aPos = GetSkirtVertPosition(tc, gl_VertexID, uHeightmapTexture);
  color = vec4(.1,.6,.1,1);
  gl_Position = uMVP * vec4(aPos, 1.0);
}
 