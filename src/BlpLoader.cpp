#include <fstream>
#include <inttypes.h>
#include <vector>

#include "BlpLoader.hpp"

// 0x00 	char[4]		always 'BLP2'
// 0x04 	uint32 		Version, always 1
// 0x08 	uint8 		Compression: 1 for uncompressed, 2 for DXTC, 3
// (cataclysm) for plain A8R8G8B8 textures (see remarks) 0x09 	uint8
// Alpha channel bit depth: 0, 1 or 8 0x0A 	uint8 		Something about
// alpha? (compressed, alpha:0 or 1): 0, (compressed, alpha:8): 1, uncompressed:
// 2,4 or 8 (mostly 8) 0x0B 	uint8 		when 0 there is only 1
// mipmaplevel. compressed: 0, 1 or 2, uncompressed: 1 or 2 (mostly 1) 0x0C
// uint32 		X resolution (power of 2) 0x10 	uint32 		Y
// resolution (power of 2) 0x14 	uint32[16] 	offsets for every mipmap
// level (or 0 when there is no more mipmap level) 0x54 	uint32[16]
// sizes for every mipmap level (or 0 when there is no more mipmap level)
enum BLPColorEncoding : uint8_t {
  COLOR_JPEG = 0, // not supported
  COLOR_PALETTE = 1,
  COLOR_DXT = 2,
  COLOR_ARGB8888 = 3,
  COLOR_ARGB8888_dup = 4, // same decompression, likely other PIXEL_FORMAT
};

enum BLPPixelFormat : uint8_t {
  PIXEL_DXT1 = 0,
  PIXEL_DXT3 = 1,
  PIXEL_ARGB8888 = 2,
  PIXEL_ARGB1555 = 3,
  PIXEL_ARGB4444 = 4,
  PIXEL_RGB565 = 5,
  PIXEL_A8 = 6,
  PIXEL_DXT5 = 7,
  PIXEL_UNSPECIFIED = 8,
  PIXEL_ARGB2565 = 9,
  PIXEL_BC5 = 11,         // DXGI_FORMAT_BC5_UNORM
  NUM_PIXEL_FORMATS = 12, // (no idea if format=10 exists)
};
struct BlpPalPixel {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t pad;
};
struct BLPHeader {
  // always 'BLP2'
  char magic[4];
  // always 1
  uint32_t formatVersion;
  BLPColorEncoding colorEncoding;
  // Alpha channel bit depth: 0, 1 or 8
  uint8_t alphaDepth;
  BLPPixelFormat pixelFormat;
  uint8_t hasMips;
  uint32_t width;
  uint32_t height;
  uint32_t mipOffsets[16]; // offsets of mipmaps (or 0 if empty)
  uint32_t mipSizes[16];   // size of mipmaps (or 0 if empty)
  union {
    BlpPalPixel palette[256];
    struct {
      uint32_t headerSize;
      char headerData[1020];
    } jpeg;
  } extended;
};

inline uint8_t expand4to8(uint8_t alpha4) { return alpha4 * 17; }

inline void rgb565to888(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = (color >> 11) & 0x1F;
  g = (color >> 5) & 0x3F;
  b = color & 0x1F;

  // Scale RGB 5:6:5 to RGB 8:8:8
  r = (r << 3) | (r >> 2); // 5-bit to 8-bit
  g = (g << 2) | (g >> 4); // 6-bit to 8-bit
  b = (b << 3) | (b >> 2); // 5-bit to 8-bit
}

void decompressDXT5Block(const uint8_t *block, uint32_t *output, int width) {
  // Read alpha data
  uint8_t alpha0 = block[0];
  uint8_t alpha1 = block[1];
  const uint64_t alphaBits = *reinterpret_cast<const uint64_t *>(block) >>
                             16; // The next 48 bits contain alpha selectors

  // Read color data
  const uint16_t color0 = *reinterpret_cast<const uint16_t *>(block + 8);
  const uint16_t color1 = *reinterpret_cast<const uint16_t *>(block + 10);
  const uint32_t colorBits = *reinterpret_cast<const uint32_t *>(block + 12);

  // Decompress alpha values
  uint8_t alphas[8];
  alphas[0] = alpha0;
  alphas[1] = alpha1;

  if (alpha0 > alpha1) {
    // Interpolate alpha (standard case)
    for (int i = 2; i < 8; ++i) {
      alphas[i] = ((8 - i) * alpha0 + (i - 1) * alpha1) / 7;
    }
  } else {
    // Special case: alpha0 <= alpha1 (1-bit alpha mode)
    for (int i = 2; i < 6; ++i) {
      alphas[i] = ((6 - i) * alpha0 + (i - 1) * alpha1) / 5;
    }
    alphas[6] = 0;
    alphas[7] = 255;
  }

  // Decode the 4x4 block of alpha
  uint8_t alpha[16];
  for (int i = 0; i < 16; ++i) {
    int selector = (alphaBits >> (i * 3)) & 0x07;
    alpha[i] = alphas[selector];
  }

  // Decompress colors
  uint8_t colors[4][3];

  // Decode color0 and color1
  rgb565to888(color0, colors[0][0], colors[0][1], colors[0][2]);
  rgb565to888(color1, colors[1][0], colors[1][1], colors[1][2]);

  if (color0 > color1) {
    // 3-color mode: interpolate colors
    for (int i = 0; i < 3; ++i) {
      colors[2][i] = (2 * colors[0][i] + colors[1][i]) /
                     3; // 2/3 of color0 + 1/3 of color1
      colors[3][i] = (colors[0][i] + 2 * colors[1][i]) /
                     3; // 1/3 of color0 + 2/3 of color1
    }
  } else {
    // 1-bit alpha mode: color2 is the average, color3 is transparent
    for (int i = 0; i < 3; ++i) {
      colors[2][i] =
          (colors[0][i] + colors[1][i]) / 2; // Average of color0 and color1
      colors[3][i] = 0;                      // Transparent
    }
  }

  // Decode the 4x4 block of colors and combine with alpha
  for (int i = 0; i < 16; ++i) {
    int x = i % 4;
    int y = i / 4;

    int colorIndex = (colorBits >> (i * 2)) & 0x3;
    uint32_t pixelIndex = y * width + x;

    output[pixelIndex] = (alpha[i] << 24) | (colors[colorIndex][2] << 16) |
                         (colors[colorIndex][1] << 8) | colors[colorIndex][0];
  }
}

void decompressDXT5(const uint8_t *compressedData, int width, int height,
                    std::vector<uint32_t> &outputBuffer) {
  int numBlocksX = (width + 3) / 4;
  int numBlocksY = (height + 3) / 4;

  outputBuffer.resize(width * height);

  for (int blockY = 0; blockY < numBlocksY; ++blockY) {
    for (int blockX = 0; blockX < numBlocksX; ++blockX) {
      const uint8_t *block =
          compressedData + (blockY * numBlocksX + blockX) * 16;
      decompressDXT5Block(block, &outputBuffer[blockY * 4 * width + blockX * 4],
                          width);
    }
  }
}

void decompressDXT3Block(const uint8_t *block, uint32_t *output, int width) {
  const uint64_t alphaData = *reinterpret_cast<const uint64_t *>(block);

  const uint16_t color0 = *reinterpret_cast<const uint16_t *>(block + 8);
  const uint16_t color1 = *reinterpret_cast<const uint16_t *>(block + 10);
  const uint32_t colorBits = *reinterpret_cast<const uint32_t *>(block + 12);

  uint8_t alpha[16];
  for (int i = 0; i < 16; ++i) {
    uint8_t alpha4 = (alphaData >> (i * 4)) & 0xF;
    alpha[i] = expand4to8(alpha4);
  }

  uint8_t colors[4][3];
  rgb565to888(color0, colors[0][0], colors[0][1], colors[0][2]);
  rgb565to888(color1, colors[1][0], colors[1][1], colors[1][2]);

  colors[2][0] = (2 * colors[0][0] + colors[1][0]) / 3;
  colors[2][1] = (2 * colors[0][1] + colors[1][1]) / 3;
  colors[2][2] = (2 * colors[0][2] + colors[1][2]) / 3;

  colors[3][0] = (colors[0][0] + 2 * colors[1][0]) / 3;
  colors[3][1] = (colors[0][1] + 2 * colors[1][1]) / 3;
  colors[3][2] = (colors[0][2] + 2 * colors[1][2]) / 3;

  for (int i = 0; i < 16; ++i) {
    int x = i % 4;
    int y = i / 4;

    int colorIndex = (colorBits >> (i * 2)) & 0x3;
    uint32_t pixelIndex = y * width + x;

    output[pixelIndex] = (alpha[i] << 24) | (colors[colorIndex][2] << 16) |
                         (colors[colorIndex][1] << 8) | colors[colorIndex][0];
  }
}

void decompressDXT3(const uint8_t *compressedData, int width, int height,
                    std::vector<uint32_t> &outputBuffer) {
  int numBlocksX = (width + 3) / 4;
  int numBlocksY = (height + 3) / 4;

  outputBuffer.resize(width * height);

  for (int blockY = 0; blockY < numBlocksY; ++blockY) {
    for (int blockX = 0; blockX < numBlocksX; ++blockX) {
      const uint8_t *block =
          compressedData + (blockY * numBlocksX + blockX) * 16;
      decompressDXT3Block(block, &outputBuffer[blockY * 4 * width + blockX * 4],
                          width);
    }
  }
}

void decompressDXT1Block(const uint8_t *block, uint32_t *output, int width,
                         bool &hasAlpha) {
  const uint16_t color0 = *reinterpret_cast<const uint16_t *>(block);
  const uint16_t color1 = *reinterpret_cast<const uint16_t *>(block + 2);
  const uint32_t colorBits = *reinterpret_cast<const uint32_t *>(block + 4);

  uint8_t colors[4][3];

  rgb565to888(color0, colors[0][0], colors[0][1], colors[0][2]);
  rgb565to888(color1, colors[1][0], colors[1][1], colors[1][2]);

  if (color0 > color1) {
    // 3-color mode: interpolate colors
    for (int i = 0; i < 3; ++i) {
      colors[2][i] = (2 * colors[0][i] + colors[1][i]) /
                     3; // 2/3 of color0 + 1/3 of color1
      colors[3][i] = (colors[0][i] + 2 * colors[1][i]) /
                     3; // 1/3 of color0 + 2/3 of color1
    }
  } else {
    // 1-bit alpha mode: color2 is the average, color3 is transparent
    for (int i = 0; i < 3; ++i) {
      colors[2][i] =
          (colors[0][i] + colors[1][i]) / 2; // Average of color0 and color1
      colors[3][i] = 0;                      // Transparent
    }
    hasAlpha = true;
  }

  // Decode the 4x4 block
  for (int i = 0; i < 16; ++i) {
    int x = i % 4;
    int y = i / 4;

    int colorIndex = (colorBits >> (i * 2)) & 0x3;
    uint32_t pixelIndex = y * width + x;

    // Combine the decompressed RGB with 255 alpha (if not transparent)
    output[pixelIndex] = (hasAlpha && colorIndex == 3)
                             ? 0
                             : (0xFF << 24) | (colors[colorIndex][2] << 16) |
                                   (colors[colorIndex][1] << 8) |
                                   colors[colorIndex][0];
  }
}

void decompressDXT1(const uint8_t *compressedData, int width, int height,
                    std::vector<uint32_t> &outputBuffer) {
  int numBlocksX = (width + 3) / 4;
  int numBlocksY = (height + 3) / 4;

  outputBuffer.resize(width * height);

  for (int blockY = 0; blockY < numBlocksY; ++blockY) {
    for (int blockX = 0; blockX < numBlocksX; ++blockX) {
      const uint8_t *block =
          compressedData + (blockY * numBlocksX + blockX) * 8;
      bool hasAlpha = false;
      decompressDXT1Block(block, &outputBuffer[blockY * 4 * width + blockX * 4],
                          width, hasAlpha);
    }
  }
}

ImageData LoadBlp(const std::string &path) {
  std::ifstream file_stream{path, std::ios::binary};
  if (!file_stream.is_open()) {
    throw std::runtime_error("Could not open file: " + path);
  }

  std::vector<unsigned char> result;

  BLPHeader header;
  file_stream.read((char *)&header, sizeof(header));
  uint buff_size = header.mipSizes[0];
  unsigned char buff[buff_size];
  file_stream.read((char *)&buff, buff_size * sizeof(buff[0]));

  if (header.colorEncoding == COLOR_PALETTE) {
    if (header.alphaDepth == 0) { // no alpha data
      result.reserve(buff_size * 3);
      for (uint i = 0; i < buff_size; i++) {
        BlpPalPixel p = header.extended.palette[buff[i]];
        result.push_back(p.r);
        result.push_back(p.g);
        result.push_back(p.b);
      }
      return ImageData(&result[0], header.width, header.height, id_RGB8);
    } else if (header.alphaDepth == 8) { // alpha data packed after color data
      unsigned char alpha_buffer[buff_size];
      file_stream.read((char *)&alpha_buffer, buff_size);
      result.reserve(buff_size * 4);
      for (uint i = 0; i < buff_size; i++) {
        BlpPalPixel p = header.extended.palette[buff[i]];
        result.push_back(p.r);
        result.push_back(p.g);
        result.push_back(p.b);
        result.push_back(alpha_buffer[i]);
      }
      return ImageData(&result[0], header.width, header.height, id_RGBA8);
    } else {
      throw std::runtime_error("Unsupported alphaDepth.");
    }
  } else if (header.colorEncoding == COLOR_DXT) {
    std::vector<uint32_t> out_buff;
    if (header.pixelFormat == PIXEL_DXT1) {
      decompressDXT1((const uint8_t *)buff, header.width, header.height,
                     out_buff);
    } else if (header.pixelFormat == PIXEL_DXT3) {
      decompressDXT3((const uint8_t *)buff, header.width, header.height,
                     out_buff);
    } else if (header.pixelFormat == PIXEL_DXT5) {
      decompressDXT5((const uint8_t *)buff, header.width, header.height,
                     out_buff);
    } else {
      throw std::runtime_error("Unsupported alphaDepth");
    }
    result.reserve(out_buff.size() * 4);
    for (auto c : out_buff) {
      result.push_back(c & 0xff);
      result.push_back(c >> 8 & 0xff);
      result.push_back(c >> 16 & 0xff);
      result.push_back(c >> 24 & 0xff);
    }
    return ImageData(&result[0], header.width, header.height, id_RGBA8);
  } else {
    throw std::runtime_error("Unsupported colorEncoding");
  }
};