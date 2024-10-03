#pragma once

#include <vector>

#include "ImageData.hpp"
#include "gl.hpp"
#include "utils.hpp"

typedef std::vector<std::pair<GLenum, GLint>> GpuTexParameters;

const GpuTexParameters kGpuTexParametersDefault{
    {GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
    {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
    {GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
    {GL_TEXTURE_MAG_FILTER, GL_LINEAR},
};

class GpuTexture {
public:
  GpuTexture() { glGenTextures(1, &m_texture); }
  GpuTexture(
      const unsigned char *data, int width, int height, int num_channels,
      const GpuTexParameters &tex_parameters_i = kGpuTexParametersDefault);
  GpuTexture(const ImageData &image, const GpuTexParameters &tex_parameters_i =
                                         kGpuTexParametersDefault)
      : GpuTexture(image.get(), image.width, image.height, image.num_channels,
                   tex_parameters_i) {};
  GpuTexture(const std::string &path, const GpuTexParameters &tex_parameters_i =
                                          kGpuTexParametersDefault)
      : GpuTexture(ImageData(path, 0)) {};

  ~GpuTexture() {
    if (m_texture != 0) {
      glDeleteTextures(1, &m_texture);
    }
  }
  NEVER_COPY(GpuTexture);
  GpuTexture(GpuTexture &&other) : m_texture{other.m_texture} {
    other.m_texture = 0;
  };

  void BindTexture(GLenum target) const;
  void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                            GLint level) const;

  static void SetTextureParameters(GLenum target,
                                   const GpuTexParameters &tex_parameters) {
    for (const auto &[e, v] : tex_parameters) {
      glTexParameteri(target, e, v);
    }
  }
  static void SetTextureData(GLenum target, const unsigned char *data,
                             GLint level, int width, int height,
                             int num_channels) {
    if (target == GL_TEXTURE_2D) {
      if (num_channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, data);
      } else if (num_channels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
      }
    } else {
      printf("Unrecognized texture target.\n");
    }
  }

private:
  GLuint m_texture;
};
