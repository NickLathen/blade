const int kBoxVerts = 10;
const int kBoxesPerStrip = 8;
const int kStripVerts = kBoxVerts * kBoxesPerStrip;
const int kBlockVerts = kStripVerts * 8;
const int kBlocksPerChunkStrip = 16;
const int kHopsPerBlock = 16;
const int kHopsPerChunk = kHopsPerBlock * 16;
const int kHeightmapBlockSize = (9 * 9) + (8 * 8);
const int kHeightNoLODSize = 9;
const int kHeightLODSize = 8;
const int kHeightmapBlockStripSize = kHeightNoLODSize +
                                     kHeightLODSize;

struct ChunkOffset {
  int block_offset;
  int block_number;
  int block_x;
  int block_y;
  int inner_x;
  int inner_y;
};

ChunkOffset GetChunkOffset(int vertexId) {
  int block_offset = vertexId % kBlockVerts;
  int block_number = vertexId / kBlockVerts;
  int block_x = block_number % kBlocksPerChunkStrip;
  int block_y = block_number / kBlocksPerChunkStrip;
  int inner_x = 0;
  int inner_y = 0;
  int box_offset = block_offset % kBoxVerts;
  int box_number = block_offset / kBoxVerts;
  int box_x = box_number % kBoxesPerStrip;
  int box_y = box_number / kBoxesPerStrip;
  inner_x += box_x * 2;
  inner_y += box_y * 2;
  switch(box_offset) {
    case 2:
      inner_y += 2;
      break;
    case 3:
    case 7:
      inner_x += 1;
      inner_y += 1;
      break;
    case 4:
      inner_x += 2;
      inner_y += 2;
      break;
    case 5:
    case 6:
      inner_x += 2;
      break;
  }
  return ChunkOffset(block_offset, block_number, block_x, block_y, inner_x, inner_y);
}

struct ChunkFracs {
  float x_frac_block;
  float y_frac_block;
  float x_frac_chunk;
  float y_frac_chunk;
};

ChunkFracs GetChunkFracs(ChunkOffset offsets) {
  float x_frac_block = float(offsets.inner_x) / float(kHopsPerBlock);
  float y_frac_block = float(offsets.inner_y) / float(kHopsPerBlock);
  float x_frac_chunk = float(offsets.block_x * kHopsPerBlock + offsets.inner_x)
                       / float(kHopsPerChunk);
  float y_frac_chunk = float(offsets.block_y * kHopsPerBlock + offsets.inner_y)
                       / float(kHopsPerChunk);
  return ChunkFracs(x_frac_block, y_frac_block, x_frac_chunk, y_frac_chunk);
};

int GetChunkIdx(ChunkOffset offsets) {
  int corner_idx = offsets.block_x * kHeightmapBlockSize * kBlocksPerChunkStrip +
                   offsets.block_y * kHeightmapBlockSize; //heightmap is column layout
  int major_hops_y = offsets.inner_y / 2;
  int minor_hops_y = offsets.inner_y % 2; //minor_hops_x will be the same
  int major_hops_x = offsets.inner_x / 2;
  int inner_component = major_hops_x * kHeightmapBlockStripSize;
  inner_component += major_hops_y + minor_hops_y * kHeightNoLODSize;
  return corner_idx + inner_component;
}

bool isHole(ChunkOffset offsets, uint holes[512]) {
  int idx = offsets.block_number * 2; //two 32bit values per block
  int box_number = offsets.block_offset / kBoxVerts;
  int box_x = box_number / kBoxesPerStrip;
  int box_y = box_number % kBoxesPerStrip;
  int shift = box_x + box_y * kBoxesPerStrip;
  if (shift > 31) {
    idx += 1; //use upper bits to check holes
    shift -= 32;
  }
  uint flag = 1u << shift;
  return (holes[idx] & flag) != 0u;
}