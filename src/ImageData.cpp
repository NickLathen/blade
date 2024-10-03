#include "ImageData.hpp"

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
  uint8_t colors[4][3]; // Store 4 colors (each 3 bytes RGB)

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

    // Combine the decompressed alpha and RGB
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
      const uint8_t *block = compressedData + (blockY * numBlocksX + blockX) *
                                                  16; // Each block is 16 bytes
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
    alpha[i] = expand4to8(alpha4); // Expand 4-bit alpha to 8-bit
  }

  uint8_t colors[4][3]; // Store 4 interpolated colors (each 3 bytes RGB)
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

  uint8_t colors[4][3]; // Store 4 colors (each 3 bytes RGB)

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
    hasAlpha = true; // Indicate that this block has transparency
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
      const uint8_t *block = compressedData + (blockY * numBlocksX + blockX) *
                                                  8; // Each block is 8 bytes
      bool hasAlpha = false;
      decompressDXT1Block(block, &outputBuffer[blockY * 4 * width + blockX * 4],
                          width, hasAlpha);
    }
  }
}

ImageData::ImageData(const std::string &filename, int req_comp) {
  unsigned char *stbi_out =
      stbi_load(filename.c_str(), &width, &height, &num_channels, req_comp);
  if (!stbi_out) {
    printf("unable to load texture=%s\n", filename.c_str());
    return;
  }
  uint size = width * height * num_channels;
  m_data.reserve(size);
  for (uint i = 0; i < size; i++) {
    m_data.push_back(stbi_out[i]);
  }
  stbi_image_free(stbi_out);
  printf("Loaded image:%s width=%d height=%d num_channels=%d\n",
         filename.c_str(), width, height, num_channels);
};

ImageData::ImageData(const unsigned char *data, int width, int height,
                     int num_channels, ImageDataFormat texture_format)
    : width{width}, height{height}, num_channels{num_channels} {
  if (texture_format == RGB8 || texture_format == RGBA8) {
    for (uint i = 0; i < width * height * num_channels; i++) {
      m_data.push_back(data[i]);
    }
    return;
  }
  std::vector<uint32_t> out_buff;
  if (texture_format == DXT1) {
    decompressDXT1((const uint8_t *)data, width, height, out_buff);
  } else if (texture_format == DXT3) {
    decompressDXT3((const uint8_t *)data, width, height, out_buff);
  } else if (texture_format == DXT5) {
    decompressDXT5((const uint8_t *)data, width, height, out_buff);
  } else {
    throw std::runtime_error("Unsupported texture_format");
  }
  // invert y axis;
  for (uint i = 0; i < width; i++) {
    for (uint j = 0; j < height / 2; j++) {
      uint yMirror = height - 1 - j;
      std::swap(out_buff[i + j * width], out_buff[i + yMirror * width]);
    }
  }
  m_data.reserve(out_buff.size() * 4);
  for (auto c : out_buff) {
    m_data.push_back(c & 0xff);
    m_data.push_back(c >> 8 & 0xff);
    m_data.push_back(c >> 16 & 0xff);
    m_data.push_back(c >> 24 & 0xff);
  }
}

ImageData ImageData::LoadBLP(const std::string &path) {
  std::ifstream file_stream(path, std::ios::binary);
  if (!file_stream.is_open()) {
    throw std::runtime_error("Could not open file: " + path);
  }

  BLPHeader header;
  file_stream.read((char *)&header, sizeof(header));
  uint buff_size = header.mipSizes[0];
  unsigned char buff[buff_size];
  file_stream.read((char *)&buff, buff_size * sizeof(buff[0]));

  if (header.colorEncoding == 1) {
    if (header.alphaDepth == 0) {
      unsigned char rgb8_buffer[buff_size * 3];
      for (uint i = 0; i < buff_size; i++) {
        BlpPalPixel p = header.extended.palette[buff[i]];
        rgb8_buffer[i * 3] = p.r;
        rgb8_buffer[i * 3 + 1] = p.g;
        rgb8_buffer[i * 3 + 2] = p.b;
      }
      // invert y axis
      for (uint i = 0; i < header.width; i++) {
        for (uint j = 0; j < header.height / 2; j++) {
          uint yMirror = header.height - 1 - j;
          for (uint k = 0; k < 3; k++) {
            std::swap(rgb8_buffer[(i + j * header.width) * 3 + k],
                      rgb8_buffer[(i + yMirror * header.width) * 3 + k]);
          }
        }
      }
      return ImageData(rgb8_buffer, (int)header.width, (int)header.height, 3,
                       RGB8);
    } else if (header.alphaDepth == 8) {
      unsigned char alpha_buffer[buff_size];
      file_stream.read((char *)&alpha_buffer, buff_size);
      unsigned char rgba8_buffer[buff_size * 4];
      for (uint i = 0; i < buff_size; i++) {
        BlpPalPixel p = header.extended.palette[buff[i]];
        rgba8_buffer[i * 4] = p.r;
        rgba8_buffer[i * 4 + 1] = p.g;
        rgba8_buffer[i * 4 + 2] = p.b;
        rgba8_buffer[i * 4 + 3] = alpha_buffer[i];
      }
      // invert y axis
      for (uint i = 0; i < header.width; i++) {
        for (uint j = 0; j < header.height / 2; j++) {
          uint yMirror = header.height - 1 - j;
          for (uint k = 0; k < 4; k++) {
            std::swap(rgba8_buffer[(i + j * header.width) * 4 + k],
                      rgba8_buffer[(i + yMirror * header.width) * 4 + k]);
          }
        }
      }
      return ImageData(rgba8_buffer, (int)header.width, (int)header.height, 4,
                       RGBA8);
    } else {
      throw std::runtime_error("Unsupported alphaDepth.");
    }
  } else if (header.colorEncoding == 2) {
    if (header.alphaDepth == 0) {
      return ImageData(buff, (int)header.width, (int)header.height, 4, DXT1);
    } else if (header.alphaDepth == 1) {
      return ImageData(buff, (int)header.width, (int)header.height, 4, DXT1);
    } else if (header.alphaDepth == 8) {
      if (header.alphaEncoding == 7) {
        return ImageData(buff, (int)header.width, (int)header.height, 4, DXT5);
      }
      return ImageData(buff, (int)header.width, (int)header.height, 4, DXT3);
    } else {
      throw std::runtime_error("Unsupported alphaDepth");
    }
  } else {
    throw std::runtime_error("Unsupported colorEncoding");
  }
};