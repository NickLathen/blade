#include "RenderPass.hpp"
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RPMaterial::RPMaterial(const std::vector<Material> &materials,
                       const std::vector<MeshVertexBuffer> &vertex_buffer_data,
                       const std::vector<GLuint> &element_buffer_data) {

  // Copy materials to GPU m_ubo
  uint num_materials = materials.size();
  BSDFMaterial material_data[num_materials];
  for (uint i = 0; i < num_materials; i++) {
    material_data[i] = materials[i].GetProperties();
  }
  m_ubo.BufferData(sizeof(material_data), material_data, GL_STATIC_DRAW);

  // copy vertex_buffer/element_buffer to GPU m_vbo/m_ebo
  m_num_elements = element_buffer_data.size();
  m_ebo.BufferData(element_buffer_data.size() * sizeof(element_buffer_data[0]),
                   &element_buffer_data[0], GL_STATIC_DRAW);
  m_vbo.BufferData(vertex_buffer_data.size() * sizeof(vertex_buffer_data[0]),
                   &vertex_buffer_data[0], GL_STATIC_DRAW);

  m_vao.BindVertexArray();
  m_vao.VertexAttribPointer(m_vbo, 0, 3, GL_FLOAT, GL_FALSE,
                            sizeof(MeshVertexBuffer), (GLvoid *)0);
  m_vao.VertexAttribPointer(m_vbo, 1, 3, GL_FLOAT, GL_FALSE,
                            sizeof(MeshVertexBuffer),
                            (GLvoid *)(offsetof(MeshVertexBuffer, normal)));
  m_vao.VertexAttribIPointer(
      m_vbo, 3, 1, GL_UNSIGNED_INT, sizeof(MeshVertexBuffer),
      (GLvoid *)(offsetof(MeshVertexBuffer, material_idx)));

  m_ebo.BindBuffer();

  m_vao.Unbind();
  m_vbo.Unbind();
  m_ebo.Unbind();
};

void RPMaterial::DrawVertices() const {
  m_vao.BindVertexArray();
  glDrawElements(GL_TRIANGLES, m_num_elements, GL_UNSIGNED_INT, 0);
  m_vao.Unbind();
};
