#pragma once

#include <unordered_map>

#include "TextureArray.hpp"

constexpr int kMultiTextureArrayDefaultSize = 256;

class MultiTextureArray {
public:
  MultiTextureArray() {};
  NEVER_COPY(MultiTextureArray);
  MultiTextureArray(MultiTextureArray &&other)
      : m_texture_array{std::move(other.m_texture_array)},
        m_texture_map{std::move(other.m_texture_map)},
        m_tex_parameters{std::move(other.m_tex_parameters)} {};
  uint32_t
  PushImage(const ImageData &image,
            const GpuTexParameters &tex_parameters = kGpuTexParametersDefault) {
    uint64_t type_id = GetImageTypeId(image, tex_parameters);
    const auto &i = m_texture_map.find(type_id);
    // If we already have a matching TextureArray, push image there
    if (i != m_texture_map.end()) {
      uint32_t idx = (*i).second[0];
      uint32_t zoffset = m_texture_array[idx].PushImage(image);
      return GetTextureId(idx, zoffset);
    }
    // Otherwise, create a new TextureArray
    m_texture_array.emplace_back(image.width, image.height, image.num_channels,
                                 kMultiTextureArrayDefaultSize, tex_parameters);
    uint32_t idx = m_texture_array.size() - 1;
    uint32_t zoffset = m_texture_array[idx].PushImage(image);
    m_texture_map[type_id] = {idx}; // TODO support multiple textures per size
    return GetTextureId(idx, zoffset);
  };
  void GenerateMipmaps() const {
    for (auto &i : m_texture_array) {
      i.GetTexture().GenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }
  }
  std::vector<GLint> BindTextures() const {
    std::vector<GLint> samplers{};
    int bindLocation = 0;
    for (auto &i : m_texture_array) {
      Bind2DArrayTextureLocation(i.GetTexture(), bindLocation);
      samplers.push_back(bindLocation++);
    }
    return samplers;
  }

private:
  inline static uint32_t GetTextureId(uint32_t idx, uint32_t zoffset) {
    return idx << 16 | zoffset;
  }
  uint64_t GetImageTypeId(const ImageData &i,
                          const GpuTexParameters &tex_parameters) {
    int param_id = -1;
    for (int i = 0; i < m_tex_parameters.size(); i++) {
      if (tex_parameters == m_tex_parameters[i]) {
        param_id = i;
        break;
      }
    }
    if (param_id == -1) {
      m_tex_parameters.push_back(tex_parameters);
      param_id = m_tex_parameters.size() - 1;
    }
    //-16bits width-
    //-16bits height-
    //-8bits num_channel-
    //-8bits param_id-
    uint64_t result = i.width;
    result <<= 16;
    result |= i.height;
    result <<= 8;
    result |= i.num_channels;
    result <<= 8;
    result |= param_id;
    return result;
  }
  std::vector<TextureArray> m_texture_array{};
  std::unordered_map<uint64_t, std::vector<size_t>> m_texture_map{};
  std::vector<GpuTexParameters> m_tex_parameters{};
};