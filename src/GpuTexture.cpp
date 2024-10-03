#include <SDL.h>

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