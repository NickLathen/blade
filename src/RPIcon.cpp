#include "RenderPass.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RPIcon::RPIcon()
    : m_shader{"shaders/icon_vertex.glsl", "shaders/icon_fragment.glsl"} {};
void RPIcon::Draw(const glm::vec4 &position, const glm::vec4 &color) const {
  m_shader.UseProgram();
  m_shader.Uniform4fv("uPos", position);
  m_shader.Uniform1ui("uColor", glm::packUnorm4x8(color));
  m_vao.BindVertexArray();
  glDrawArrays(GL_POINTS, 0, 1);
  m_vao.Unbind();
  glUseProgram(0);
};
