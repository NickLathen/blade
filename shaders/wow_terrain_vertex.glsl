#version 320 es
precision highp float;

#include "wow_terrain_functions.glsl"

uniform mat4 uMVP;
uniform vec2 uCornerPos;

struct HeightNormalMap {
    float height;
    uint packed_normal;
    float heightA;
    uint packed_normalA;
};

layout(std430, binding=0) buffer uHeightmapBuffer {
    HeightNormalMap uHeight[145 * 256];
};

out vec3 worldPos;
out vec2 texCoords;
out vec3 normal;
flat out int blockNumber;

void main() {
  ChunkOffset offsets = GetChunkOffset(gl_VertexID);
  ChunkFracs fracs = GetChunkFracs(offsets);
  blockNumber = offsets.block_number;
  texCoords = vec2(fracs.x_frac_block, fracs.y_frac_block);
  int chunk_idx = GetChunkIdx(offsets);
  float height;
  uint packed_normal;
  if (chunk_idx % 2 == 0) {
    height = uHeight[chunk_idx / 2].height;
    packed_normal = uHeight[chunk_idx / 2].packed_normal;
  } else {
    height = uHeight[chunk_idx / 2].heightA;
    packed_normal = uHeight[chunk_idx / 2].packed_normalA;
  }
  vec3 aPos = vec3(-fracs.x_frac_chunk * 533.33333 + uCornerPos.x,
                   height,
                   (fracs.y_frac_chunk - 1.0) * 533.33333 - uCornerPos.y);
  normal = unpackSnorm4x8(packed_normal).xyz;
  worldPos = aPos;
  gl_Position = uMVP * vec4(aPos, 1.0);
}
 