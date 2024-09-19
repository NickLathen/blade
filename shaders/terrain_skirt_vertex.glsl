#version 300 es

#include "structs.glsl"
#include "terrain_functions.glsl"

uniform sampler2D uHeightmapTexture;
uniform mat4 uMVP;

layout(std140) uniform uTileConfigBlock {
  TileConfig tileConfig;
} uTileConfig;

flat out vec4 color;
void main() {
  TileConfig tc = uTileConfig.tileConfig;
  vec3 aPos = GetHeightmapSkirtPosition(tc, uHeightmapTexture, gl_VertexID);
  color = vec4(.1,.6,.1,1);
  gl_Position = uMVP * vec4(aPos, 1.0);
}
