#include "RenderPass.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RP_Icon::RP_Icon()
    : mIconShader{"shaders/icon_vertex.glsl", "shaders/icon_fragment.glsl"} {};
void RP_Icon::draw(const glm::vec4 &position, const glm::vec4 &color) const {
  mIconShader.useProgram();
  mIconShader.uniform4fv("uPos", position);
  mIconShader.uniform1ui("uColor", glm::packUnorm4x8(color));
  mVAO.bindVertexArray();
  glDrawArrays(GL_POINTS, 0, 1);
  mVAO.unbind();
  glUseProgram(0);
};
