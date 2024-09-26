#include "RenderPass.hpp"

void EnableAnisotropicFilter(const RPTexture &texture) {
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

RPTexture loadTexture2D(unsigned char *data, int width, int height,
                        int num_channels) {
  RPTexture texture{};
  texture.BindTexture(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  EnableAnisotropicFilter(texture);

  if (data != 0) {
    if (num_channels == 3) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, data);
    } else if (num_channels == 4) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    printf("Failed to load texture\n");
  }
  return texture;
}