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

layout(std430, binding=3) buffer uHolesBuffer {
  uint uHoles[512];
};

out vec3 worldPos;
out vec2 texCoords;
out vec3 normal;
flat out int blockNumber;

void main() {
  ChunkOffset offsets = GetChunkOffset(gl_VertexID);
  if (isHole(offsets, uHoles)) {
    offsets.inner_x = 0;
    offsets.inner_y = 0;
  }
  blockNumber = offsets.block_number;
  ChunkFracs fracs = GetChunkFracs(offsets);
  texCoords = vec2(fracs.x_frac_block, fracs.y_frac_block);
  int chunk_idx = GetChunkIdx(offsets);
  float height;
  uint packed_normal;
  int iseven = int(chunk_idx % 2 == 0);
  int isodd = int(chunk_idx % 2 == 1);
  HeightNormalMap map = uHeight[chunk_idx  / 2];
  height = float(iseven) * map.height + float(isodd) * map.heightA;
  packed_normal = uint(iseven) * map.packed_normal + uint(isodd) * map.packed_normalA;
  vec3 aPos = vec3(-fracs.x_frac_chunk * 533.33333 + uCornerPos.x,
                   height,
                   (fracs.y_frac_chunk - 1.0) * 533.33333 - uCornerPos.y);
  normal = unpackSnorm4x8(packed_normal).xyz;
  worldPos = aPos;
  gl_Position = uMVP * vec4(aPos, 1.0);
}
 