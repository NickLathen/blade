#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "ImageData.hpp"
#include "MeshGroup.hpp"
#include "RenderPass.hpp"
#include "utils.hpp"

RPTexturedMaterial::RPTexturedMaterial(
    const std::vector<std::string> &texture_files,
    const std::vector<TextureVertexData> &vertex_buffer_data,
    const std::vector<GLuint> &element_buffer_data,
    const std::vector<MeshMap> &mesh_map) {
  m_num_elements = element_buffer_data.size();

  for (const auto &texture_file : texture_files) {
    ImageData image{texture_file, 0};
    m_textures.emplace_back(loadTexture2D(image.get(), image.width,
                                          image.height, image.num_channels));
  }

  PrecomputeDrawCalls(mesh_map);

  // copy vertex_buffer/element_buffer to GPU m_vbo/m_ebo
  m_ebo.BufferData(element_buffer_data.size() * sizeof(element_buffer_data[0]),
                   &element_buffer_data[0], GL_STATIC_DRAW);
  m_vbo.BufferData(vertex_buffer_data.size() * sizeof(vertex_buffer_data[0]),
                   &vertex_buffer_data[0], GL_STATIC_DRAW);

  m_vao.BindVertexArray();
  m_vao.VertexAttribPointer(m_vbo, 0, 3, GL_FLOAT, GL_FALSE,
                            sizeof(TextureVertexData), (GLvoid *)0);
  m_vao.VertexAttribPointer(m_vbo, 1, 3, GL_FLOAT, GL_FALSE,
                            sizeof(TextureVertexData),
                            (GLvoid *)(offsetof(TextureVertexData, normal)));
  m_vao.VertexAttribPointer(
      m_vbo, 2, 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertexData),
      (GLvoid *)(offsetof(TextureVertexData, tex_coords)));
  m_vao.VertexAttribIPointer(
      m_vbo, 4, 1, GL_INT, sizeof(TextureVertexData),
      (GLvoid *)(offsetof(TextureVertexData, texture_idx)));

  m_ebo.BindBuffer();

  m_vao.Unbind();
  m_vbo.Unbind();
  m_ebo.Unbind();
};

void RPTexturedMaterial::PrecomputeDrawCalls(
    const std::vector<MeshMap> &mesh_maps) {
  // batch draw calls to accomodate texture limits,
  // this requires that the meshes are presorted by texture usage
  const int kTextureLimit = 32;
  int firstVertex = 0;
  int lastVertex = 0;
  int firstTexture = 0;
  m_draw_calls.clear();
  for (int i = 0; i < mesh_maps.size(); i++) {
    const MeshMap &mesh_map{mesh_maps[i]};
    // if the current mesh overflows the texture limit
    if (mesh_map.texture_idx > firstTexture + kTextureLimit) {
      // set current mesh as end of draw range
      lastVertex = mesh_map.element_offset;
      // bind textures up to limit
      // draw elements that use textures within limit
      m_draw_calls.push_back({firstTexture, firstTexture + kTextureLimit,
                              firstVertex, lastVertex});
      firstVertex = lastVertex;
      firstTexture += kTextureLimit;
    }
  }
  // draw final meshes that didn't overflow texture limit in loop
  m_draw_calls.push_back(
      {firstTexture, (int)m_textures.size(), firstVertex, (int)m_num_elements});
};

void RPTexturedMaterial::DrawVertices(const RPMaterialShader &shader) const {
  m_vao.BindVertexArray();

  for (auto &c : m_draw_calls) {
    shader.BindTextures(m_textures.begin() + c.firstTexture,
                        m_textures.begin() + c.lastTexture);
    glDrawElements(GL_TRIANGLES, c.lastVertex - c.firstVertex, GL_UNSIGNED_INT,
                   (void *)(c.firstVertex * sizeof(GLuint)));
  }

  m_vao.Unbind();
};
