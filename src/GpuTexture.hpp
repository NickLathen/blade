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
  GpuTexture(const unsigned char *data, int width, int height, int num_channels,
             const GpuTexParameters &tex_parameters = kGpuTexParametersDefault);
  GpuTexture(const std::vector<ImageData> &images,
             const GpuTexParameters &tex_parameters = kGpuTexParametersDefault);
  GpuTexture(int width, int height, int depth, int num_channels,
             const GpuTexParameters &tex_parameters = kGpuTexParametersDefault);
  GpuTexture(const ImageData &image,
             const GpuTexParameters &tex_parameters = kGpuTexParametersDefault)
      : GpuTexture(image.get(), image.width, image.height, image.num_channels,
                   tex_parameters) {};
  GpuTexture(const std::string &path,
             const GpuTexParameters &tex_parameters = kGpuTexParametersDefault)
      : GpuTexture(ImageData(path, 0), tex_parameters) {};
  static GpuTexture
  FromPaths(const std::vector<std::string> &paths,
            const GpuTexParameters &tex_parameters = kGpuTexParametersDefault);

  ~GpuTexture() {
    if (m_texture != 0) {
      glDeleteTextures(1, &m_texture);
    }
  }
  NEVER_COPY(GpuTexture);
  GpuTexture(GpuTexture &&other) : m_texture{other.m_texture} {
    other.m_texture = 0;
  };
  void GenerateMipmap(GLenum target) const;
  void BindTexture(GLenum target) const;
  void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                            GLint level) const;

  static void SetTextureParameters(GLenum target,
                                   const GpuTexParameters &tex_parameters) {
    for (const auto &[e, v] : tex_parameters) {
      glTexParameteri(target, e, v);
    }
  }
  static void SetTextureData2D(GLenum target, const unsigned char *data,
                               GLint level, int width, int height,
                               int num_channels) {
    if (target == GL_TEXTURE_2D) {
      if (num_channels == 1) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED,
                     GL_UNSIGNED_BYTE, data);
      } else if (num_channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, data);
      } else if (num_channels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
      } else {
        throw std::runtime_error("Unsupported num_channels.");
      }
    } else {
      throw std::runtime_error("Unrecognized texture target.");
    }
  }
  static void SetTextureData3D(GLenum target, const unsigned char *data,
                               GLint level, int width, int height, int depth,
                               int num_channels) {
    if (target == GL_TEXTURE_2D) {
      if (num_channels == 1) {
        glTexImage3D(GL_TEXTURE_2D, 0, GL_R8, width, height, depth, 0, GL_RED,
                     GL_UNSIGNED_BYTE, data);
      } else if (num_channels == 3) {
        glTexImage3D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, depth, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, data);
      } else if (num_channels == 4) {
        glTexImage3D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, depth, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, data);
      } else {
        throw std::runtime_error("Unsupported num_channels.");
      }
    } else {
      throw std::runtime_error("Unrecognized texture target.");
    }
  }
  static void SetTextureSubData3D(GLenum target, const unsigned char *data,
                                  GLint level, int width, int height,
                                  int zoffset, int num_channels) {
    if (target == GL_TEXTURE_2D_ARRAY) {
      if (num_channels == 1) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, zoffset, width, height, 1,
                        GL_RED, GL_UNSIGNED_BYTE, data);
      } else if (num_channels == 3) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, zoffset, width, height, 1,
                        GL_RGB, GL_UNSIGNED_BYTE, data);
      } else if (num_channels == 4) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, zoffset, width, height, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE, data);
      } else {
        throw std::runtime_error("Unsupported num_channels.");
      }
    } else {
      throw std::runtime_error("Unrecognized texture target.");
    }
  }
  static void SetTextureSubData3D(GLenum target, ImageData &image,
                                  int zoffset = 0) {
    SetTextureSubData3D(target, image.get(), 0, image.width, image.height,
                        zoffset, image.num_channels);
  }
  static void SetTextureSubData3D(GLenum target, std::vector<ImageData> &images,
                                  int offset = 0) {
    for (int depth = 0; depth < images.size(); depth++) {
      ImageData &image = images[depth];
      SetTextureSubData3D(target, image.get(), 0, image.width, image.height,
                          depth + offset, image.num_channels);
    }
  }

private:
  GLuint m_texture;
};
