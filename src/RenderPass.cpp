#include "RenderPass.hpp"

void BindTextureLocation(const GpuTexture &texture,
                         const GLuint texture_location) {
  glActiveTexture(GL_TEXTURE0 + texture_location);
  texture.BindTexture(GL_TEXTURE_2D);
}
void Bind2DArrayTextureLocation(const GpuTexture &texture,
                                const GLuint texture_location) {
  glActiveTexture(GL_TEXTURE0 + texture_location);
  texture.BindTexture(GL_TEXTURE_2D_ARRAY);
}