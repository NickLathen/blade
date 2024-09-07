#include "RenderPass.hpp"
#include <PerlinNoise.hpp>

struct RPTerrainVertexBuffer {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 tex_coords;
};

float Rescale(float out_min, float out_max, float value, float in_min,
              float in_max) {
  if (value <= in_min)
    return out_min;
  if (value >= in_max)
    return out_max;
  float range = in_max - in_min;
  return out_min + (out_max - out_min) * (value - in_min) / range;
}

RPTerrain::RPTerrain() {
  BSDFMaterial terrain_material{
      .ambient_color = glm::vec3(0.1, 0.1, 0.1),
      .diffuse_color = glm::vec3(.8, 0.3, 0.0),
      .specular_color = glm::vec3(0.5, 0.5, 0.5),
  };
  m_ubo.BufferData(sizeof(terrain_material), &terrain_material, GL_STATIC_DRAW);

  // build grid
  // y = sin(x) + cos(z)
  // dy/dx = cos(x)
  // dy/dz = -sin(z)
  glm::vec2 x_range{-32.0, 32.0};
  glm::vec2 z_range{-32.0, 32.0};
  uint width = 256;
  uint depth = 256;
  RPTerrainVertexBuffer terrain_buffer[width * depth];
  const siv::PerlinNoise::seed_type seed = 123456u;
  const siv::PerlinNoise perlin{seed};
  for (uint i = 0; i < depth; i++) {
    for (uint j = 0; j < width; j++) {
      float x_coord = Rescale(x_range.x, x_range.y, j, 0, width);
      float z_coord = Rescale(z_range.x, z_range.y, i, 0, depth);
      float epsilon = .0001;
      float scale = 1.0f / 8.0f;
      float perlin_x = (x_coord + 32.0) * scale * sin(.7);
      float perlin_z = (z_coord + 32.0) * scale * -cos(.7);

      float py = perlin.noise2D(perlin_x, perlin_z);
      float pyx = perlin.noise2D(perlin_x + epsilon, perlin_z);
      float pyz = perlin.noise2D(perlin_x, perlin_z + epsilon);
      float y = 0.0f * py;

      glm::vec3 position{x_coord, y - 5.0f, z_coord};

      float dy_dx = (pyx - py) / epsilon;
      float dy_dz = -(pyz - py) / epsilon;
      glm::vec3 x_tangent{1.0, dy_dx, 0.0};
      glm::vec3 y_tangent{0.0, dy_dz, 1.0};
      glm::vec3 normal = glm::normalize(glm::cross(y_tangent, x_tangent));

      glm::vec2 tex_coords{1.0 * j / width, 1.0 * i / depth};

      terrain_buffer[i * depth + j] = {position, normal, tex_coords};
    }
  }
  m_vbo.BufferData(sizeof(terrain_buffer), terrain_buffer, GL_STATIC_DRAW);
  uint num_strips = depth - 1;
  uint vertices_per_strip = 2 * width;
  uint degenerate_vertices = 2 * (depth - 2);
  uint total_elements = num_strips * vertices_per_strip + degenerate_vertices;
  std::vector<GLuint> element_buffer{};
  element_buffer.reserve(total_elements);
  for (uint i = 0; i < depth - 1; i++) {
    uint row_offset = i * width;
    uint next_row_offset = row_offset + width;
    for (uint j = 0; j < width; j++) {
      element_buffer.push_back(row_offset + j);
      element_buffer.push_back(next_row_offset + j);
    }
    if (i < depth - 2) {
      // add degenerate triangles to get back to beginning of next row
      element_buffer.push_back(next_row_offset + width - 1);
      element_buffer.push_back(next_row_offset);
    }
  }

  m_num_elements = element_buffer.size();
  m_ebo.BufferData(m_num_elements * sizeof(element_buffer[0]),
                   &element_buffer[0], GL_STATIC_DRAW);
  m_vao.BindVertexArray();
  GLint stride = sizeof(RPTerrainVertexBuffer);
  m_vao.VertexAttribPointer(m_vbo, 0, 3, GL_FLOAT, GL_FALSE, stride,
                            (GLvoid *)0);
  m_vao.VertexAttribPointer(
      m_vbo, 1, 3, GL_FLOAT, GL_FALSE, stride,
      (GLvoid *)(offsetof(RPTerrainVertexBuffer, normal)));
  m_vao.VertexAttribPointer(
      m_vbo, 2, 3, GL_FLOAT, GL_FALSE, stride,
      (GLvoid *)(offsetof(RPTerrainVertexBuffer, tex_coords)));
  glVertexAttribI4ui(3, 0, 0, 0, 0); // aMaterialIdx
  m_ebo.BindBuffer();

  m_vao.Unbind();
  m_vbo.Unbind();
  m_ebo.Unbind();
}
void RPTerrain::DrawVertices() const {
  m_vao.BindVertexArray();
  glDrawElements(GL_TRIANGLE_STRIP, m_num_elements, GL_UNSIGNED_INT, 0);
  m_vao.Unbind();
};
