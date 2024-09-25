#pragma once

#include <memory>
#include <stb_image.h>
#include <string>

class ImageData {
  struct StbiImageDeleter {
    void operator()(void *p) const { stbi_image_free(p); }
  };

public:
  ImageData(const std::string &filename, int req_comp)
      : data{stbi_load(filename.c_str(), &width, &height, &num_channels,
                       req_comp)} {};

  unsigned char *get() const { return data.get(); };
  int width;
  int height;
  int num_channels;

private:
  std::unique_ptr<unsigned char, StbiImageDeleter> data;
};