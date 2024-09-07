#include "RenderPass.hpp"

RPTex::RPTex()
    : m_shader{"shaders/texture_vertex.glsl", "shaders/texture_fragment.glsl"} {
  m_shader.UseProgram();
  m_shader.Uniform1i("uTexture", m_texture_binding);
  glUseProgram(0);

  struct TexVert {
    glm::vec3 position;  // -1 to 1
    glm::vec2 tex_coord; // 0 to 1
  };
  TexVert tex_verts[3]{
      {glm::vec3(-1.0, 3.0, 0.0f), glm::vec2(0.0, 2.0)},  // upper left
      {glm::vec3(-1.0, -1.0, 0.0f), glm::vec2(0.0, 0.0)}, // bottom left
      {glm::vec3(3.0, -1.0, 0.0f), glm::vec2(2.0, 0.0)}}; // bottom right
  m_vbo.BufferData(sizeof(tex_verts), &tex_verts[0], GL_STATIC_DRAW);

  m_vao.BindVertexArray();
  m_vao.VertexAttribPointer(m_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5,
                            (void *)0);
  m_vao.VertexAttribPointer(m_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5,
                            (void *)(sizeof(float) * 3));
  m_vao.Unbind();
};
void RPTex::Draw(const RPTexture &texture) const {
  m_shader.UseProgram();
  glActiveTexture(GL_TEXTURE0 + m_texture_binding);
  texture.BindTexture(GL_TEXTURE_2D);
  m_vao.BindVertexArray();
  glDrawArrays(GL_TRIANGLES, 0, 3);
  m_vao.Unbind();
  glUseProgram(0);
};
