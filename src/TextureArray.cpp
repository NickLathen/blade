#include "TextureArray.hpp"

TextureArray::TextureArray(int width, int height, int num_channels,
                           int capacity, GpuTexParameters tex_parameters)
    : m_texture{width, height, capacity, num_channels, tex_parameters},
      m_width{width}, m_height{height}, m_num_channels{num_channels},
      m_capacity{capacity} {};
uint32_t TextureArray::PushImage(ImageData &image) {
  if (image.width != m_width || image.height != m_height ||
      image.num_channels != m_num_channels) {
    printf("Image doesn't match TextureArray dimensions.");
    return 0;
  }
  if (m_cur_depth == m_capacity) {
    throw std::runtime_error("TextureArray out of depth slots.");
  }
  m_texture.BindTexture(GL_TEXTURE_2D_ARRAY);
  m_texture.SetTextureSubData3D(GL_TEXTURE_2D_ARRAY, image, (int)m_cur_depth);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  return m_cur_depth++;
};
