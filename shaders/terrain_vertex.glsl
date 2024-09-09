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
};
layout(std140) uniform uTileConfigBlock {
  TileConfig tileConfig;
} uTileConfig;

layout (location = 0) in uint aMaterialIdx;


out vec3 worldPos;
out vec2 texCoords;
out vec2 terrainCoords;
out vec3 normalDir;
out vec4 lightSpacePosition;
flat out uint materialIdx;

vec3 GetTerrainPosition(vec2 texCoords, float gridScale) {
  return vec3(
    (texCoords.x - 0.5f) * gridScale,
    0.0f,
    -(texCoords.y - 0.5f) * gridScale
  );
}

vec2 GetGridTexCoords(int i, int j, int resolution) {
  return vec2(
    float(j) / float(resolution),
    float(i) / float(resolution)
  );
}

vec3 GetGradient(sampler2D tex, vec2 coords, float coordsScale) {
  float epsilon = 0.001f;
  float gradScale = coordsScale / epsilon;
  float heightCenter = texture(tex, coords).r;
  float heightRight  = texture(tex, coords + vec2(epsilon, 0.0)).r;
  float heightUp     = texture(tex, coords + vec2(0.0, epsilon)).r;
  float dy_dx = -(heightRight - heightCenter) * gradScale;
  float dy_dz = (heightUp - heightCenter) * gradScale;
  return normalize(vec3(dy_dx, 1.0f, dy_dz));
}

void main() {
  TileConfig tc = uTileConfig.tileConfig;
  //texCoords -> aPos
  int rowWidth = tc.resolution * 2 + 2;
  int row = gl_VertexID / rowWidth;
  int rowVert = gl_VertexID % rowWidth;
  if (rowVert < rowWidth - 2) {
    texCoords = GetGridTexCoords(rowVert / 2, row + rowVert % 2, tc.resolution);
  } else if (rowVert == rowWidth - 2) {
    texCoords = GetGridTexCoords(tc.resolution - 1, row + 1, tc.resolution);
  } else if (rowVert == rowWidth - 1) {
    texCoords = GetGridTexCoords(0, row + 1, tc.resolution);
  }
  vec3 aPos = GetTerrainPosition(texCoords, tc.grid_scale);
  terrainCoords = texCoords * tc.width_scale;
  float height = texture(uHeightmapTexture, terrainCoords).r;
  height *= tc.height_scale;
  vec3 position = vec3(aPos.x, height, aPos.z);
  worldPos = (uModelMatrix * vec4(position, 1.0)).xyz;
  materialIdx = aMaterialIdx;
  gl_Position = uMVP * vec4(position, 1.0);
  lightSpacePosition = uLightMVP * vec4(position, 1.0);

  float coordsScale = tc.height_scale / tc.width_scale / tc.grid_scale;
  normalDir = GetGradient(uHeightmapTexture, terrainCoords, coordsScale);
  normalDir = mat3(uModelMatrix) * normalDir;
}
 