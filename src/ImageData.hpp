#pragma once

#include <fstream>
#include <memory>
#include <sstream>
#include <stb_image.h>
#include <string>
#include <vector>

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
#pragma pack(push, 1)
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
  // 1 for uncompressed, 2 for DXTC, 3 for A8R8G8B8
  uint8_t colorEncoding;
  // Alpha channel bit depth: 0, 1 or 8
  uint8_t alphaDepth;
  // Alpha encoding 7 indicates DXT 5,
  // otherwise, DXT3 for 8bit and DXT1 for 1bit
  uint8_t alphaEncoding;
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
#pragma pack(pop)

enum ImageDataFormat { RGBA8, RGB8, DXT5, DXT3, DXT1, A8R8G8B8 };

class ImageData {
  struct StbiImageDeleter {
    void operator()(void *p) const { stbi_image_free(p); }
  };

public:
  ImageData(const std::string &filename, int req_comp = 0);
  ImageData(const unsigned char *data, int width, int height, int num_channels,
            ImageDataFormat texture_format);
  static ImageData LoadBLP(const std::string &path);
  const unsigned char *get() const { return m_data.data(); };
  int width;
  int height;
  int num_channels;

private:
  std::vector<unsigned char> m_data;
};