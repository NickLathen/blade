#include <SDL.h>
#include <iostream>

#include "GpuTexture.hpp"

GpuTexture::GpuTexture(const unsigned char *data, int width, int height,
                       int num_channels,
                       const GpuTexParameters &tex_parameters) {
  glGenTextures(1, &m_texture);
  BindTexture(GL_TEXTURE_2D);
  SetTextureParameters(GL_TEXTURE_2D, tex_parameters);
  if (data != 0) {
    SetTextureData(GL_TEXTURE_2D, data, 0, width, height, num_channels);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    printf("Failed to load texture\n");
  }
};
GpuTexture::GpuTexture(const std::vector<ImageData> &images,
                       const GpuTexParameters &tex_parameters) {
  glGenTextures(1, &m_texture);
  BindTexture(GL_TEXTURE_2D_ARRAY);
  SetTextureParameters(GL_TEXTURE_2D, tex_parameters);

  int texture_depth = images.size();
  int texture_width = 0;
  int texture_height = 0;
  int num_channels = 0;
  GLint internalFormat;
  GLenum format;
  for (int i = 0; i < texture_depth; ++i) {
    const ImageData &image = images[i];
    if (i == 0) {
      texture_width = image.width;
      texture_height = image.height;
      num_channels = image.num_channels;
      if (num_channels == 1) {
        internalFormat = GL_R8;
        format = GL_RED;
      } else if (num_channels == 3) {
        internalFormat = GL_RGB8;
        format = GL_RGB;
      } else if (num_channels == 4) {
        internalFormat = GL_RGBA8;
        format = GL_RGBA;
      } else {
        throw std::runtime_error("Unsupported channels");
      }
      glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, texture_width,
                   texture_height, texture_depth, 0, format, GL_UNSIGNED_BYTE,
                   nullptr);
    }
    if (image.width != texture_width || image.height != texture_height ||
        image.num_channels != num_channels) {
      std::cerr << "Image dimensions do not match!" << std::endl;
      break;
    }
    if (!image.get()) {
      std::cerr << "Failed to load image!" << std::endl;
      break;
    }
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, image.width, image.height,
                    1, format, GL_UNSIGNED_BYTE, image.get());
  }
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
};
GpuTexture GpuTexture::FromPaths(const std::vector<std::string> &paths,
                                 const GpuTexParameters &tex_parameters) {
  std::vector<ImageData> images;
  for (const auto &p : paths) {
    images.emplace_back(p, 4);
  }
  return GpuTexture{images, tex_parameters};
};
void GpuTexture::BindTexture(GLenum target) const {
  glBindTexture(target, m_texture);
};
void GpuTexture::FramebufferTexture2D(GLenum target, GLenum attachment,
                                      GLenum textarget, GLint level) const {
  glFramebufferTexture2D(target, attachment, textarget, m_texture, level);
};

void EnableAnisotropicFilter(const GpuTexture &texture) {
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
  const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
  GLfloat maxAnisotropy = 0.0f;
  if (strstr(extensions, "GL_EXT_texture_filter_anisotropic")) {
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
                &maxAnisotropy); // Get the max anisotropy
  } else {
    printf("Anisotropic filtering is not supported.\n");
    return;
  }
  typedef void (*glTexParameterfEXT_t)(GLenum target, GLenum pname,
                                       GLfloat param);
  glTexParameterfEXT_t glTexParameterfEXT =
      (glTexParameterfEXT_t)SDL_GL_GetProcAddress("glTexParameterfEXT");

  if (glTexParameterfEXT) {
    texture.BindTexture(GL_TEXTURE_2D);
    glTexParameterfEXT(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                       maxAnisotropy);
  } else {
    printf("Anisotropic filtering function not available.\n");
  }
#undef GL_TEXTURE_MAX_ANISOTROPY_EXT
#undef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
}