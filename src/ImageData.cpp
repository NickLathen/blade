#include "ImageData.hpp"

ImageData::ImageData(const std::string &filename, int req_comp) {
  unsigned char *stbi_out =
      stbi_load(filename.c_str(), &width, &height, &num_channels, req_comp);
  if (!stbi_out) {
    printf("unable to load texture=%s\n", filename.c_str());
    return;
  }
  size_t size = width * height * num_channels;
  m_data.insert(m_data.end(), stbi_out, stbi_out + size);
  stbi_image_free(stbi_out);
  printf("Loaded image:%s width=%d height=%d num_channels=%d\n",
         filename.c_str(), width, height, num_channels);
};

ImageData::ImageData(const unsigned char *data, int width, int height,
                     ImageDataFormat texture_format)
    : width{width}, height{height} {
  if (texture_format == id_RGB8 || texture_format == id_RGBA8) {
    if (texture_format == id_RGB8) {
      num_channels = 3;
    } else {
      num_channels = 4;
    }
    size_t size = width * height * num_channels;
    m_data.insert(m_data.end(), data, data + size);
    return;
  } else if (texture_format == id_R8) {
    num_channels = 1;
    size_t size = width * height;
    m_data.insert(m_data.end(), data, data + size);
  } else {
    throw std::runtime_error("Unsupported texture_format");
  }
}