#version 300 es

#include "structs.glsl"
#include "terrain_functions.glsl"

uniform sampler2D uHeightmapTexture;

uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat4 uLightMVP;

layout(std140) uniform uTileConfigBlock {
  TileConfig tileConfig;
} uTileConfig;

layout (location = 0) in uint aMaterialIdx;

out vec3 worldPos;
out vec2 texCoords;
out vec2 heightmapCoords;
out vec4 lightSpacePosition;
flat out uint materialIdx;

void main() {
  TileConfig tc = uTileConfig.tileConfig;
  texCoords = GetHeightmapTexCoords(tc, gl_VertexID);
  heightmapCoords = GetHeightmapCoords(tc, texCoords);
  vec3 aPos = GetHeightmapPosition(tc, uHeightmapTexture, texCoords, heightmapCoords);
  worldPos = (uModelMatrix * vec4(aPos, 1.0)).xyz;
  materialIdx = aMaterialIdx;
  gl_Position = uMVP * vec4(aPos, 1.0);
  lightSpacePosition = uLightMVP * vec4(aPos, 1.0);
}
 