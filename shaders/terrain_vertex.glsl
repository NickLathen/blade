#version 300 es

uniform sampler2D uHeightmapTexture;

uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat4 uLightMVP;

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

layout (location = 0) in uint aMaterialIdx;

out vec3 worldPos;
out vec2 texCoords;
out vec2 heightmapCoords;
out vec4 lightSpacePosition;
flat out uint materialIdx;

ivec2 GetVertGridCoords(int vertexId, int resolution) {
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

vec2 GetVertTexCoords(TileConfig tc, int vertexId) {
  ivec2 gridCoords = GetVertGridCoords(vertexId, tc.resolution);
  return GetGridTexCoords(gridCoords, tc.resolution);
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

void main() {
  TileConfig tc = uTileConfig.tileConfig;
  //texCoords -> aPos
  texCoords = GetVertTexCoords(tc, gl_VertexID);
  heightmapCoords = GetHeightmapCoords(tc, texCoords);
  vec3 aPos = GetTerrainPosition(tc, texCoords, uHeightmapTexture, heightmapCoords);
  worldPos = (uModelMatrix * vec4(aPos, 1.0)).xyz;
  materialIdx = aMaterialIdx;
  gl_Position = uMVP * vec4(aPos, 1.0);
  lightSpacePosition = uLightMVP * vec4(aPos, 1.0);
}
 