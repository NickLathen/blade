#include "RenderPass.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RP_Icon::RP_Icon()
    : mIconShader{"shaders/icon_vertex.glsl", "shaders/icon_fragment.glsl"} {
  mVAO.bindVertexArray();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  mVAO.unbind();
};
void RP_Icon::draw(const glm::vec4 &position, const glm::vec4 &color) const {
  mIconShader.useProgram();
  mVAO.bindVertexArray();
  glUniform4fv(mIconShader.getUniformLocation("uPos"), 1,
               glm::value_ptr(position));
  glUniform1ui(mIconShader.getUniformLocation("uColor"),
               glm::packUnorm4x8(color));
  glDrawArrays(GL_POINTS, 0, 1);
  mVAO.unbind();
  glUseProgram(0);
};
