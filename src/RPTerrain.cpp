#include "RenderPass.hpp"
#include <PerlinNoise.hpp>

RPTerrain::RPTerrain() {
  BSDFMaterial terrain_material{
      .ambient_color = glm::vec3(0.5, 0.5, 0.5),
      .diffuse_color = glm::vec3(.8, 0.3, 0.0),
      .specular_color = glm::vec3(1.0, 1.0, 1.0),
  };
  m_ubo.BufferData(sizeof(terrain_material), &terrain_material, GL_STATIC_DRAW);
  m_vao.BindVertexArray();
  glVertexAttribI4ui(0, 0, 0, 0, 0); // aMaterialIdx
  m_vao.Unbind();
}
void RPTerrain::DrawVertices(int resolution) const {
  m_vao.BindVertexArray();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, resolution * (resolution * 2 + 2));
  m_vao.Unbind();
};
