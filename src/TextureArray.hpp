#pragma once

#include "GpuTexture.hpp"

class TextureArray {
public:
  TextureArray(
      int width, int height, int num_channels, int capacity,
      const GpuTexParameters &tex_parameters = kGpuTexParametersDefault);
  NEVER_COPY(TextureArray);
  TextureArray(TextureArray &&other)
      : m_texture{std::move(other.m_texture)}, m_cur_depth{other.m_cur_depth},
        m_width{other.m_width}, m_height{other.m_height},
        m_num_channels{other.m_num_channels}, m_capacity{other.m_capacity} {};
  uint32_t PushImage(const ImageData &image);
  inline const GpuTexture &GetTexture() const { return m_texture; };

private:
  GpuTexture m_texture;
  uint32_t m_cur_depth{0};
  const GLsizei m_width;
  const GLsizei m_height;
  const GLsizei m_num_channels;
  const GLsizei m_capacity;
};