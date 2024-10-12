#pragma once

#include <fstream>
#include <memory>
#include <sstream>
#include <stb_image.h>
#include <string>
#include <vector>

enum ImageDataFormat {
  id_RGBA8,
  id_RGB8,
  id_R8,
};

class ImageData {
public:
  ImageData(const std::string &filename, int req_comp = 0);
  ImageData(const unsigned char *data, int width, int height,
            ImageDataFormat texture_format);
  const unsigned char *get() const { return m_data.data(); };
  int width;
  int height;
  int num_channels;

private:
  std::vector<unsigned char> m_data;
};