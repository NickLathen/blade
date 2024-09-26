#pragma once
#include "gl.hpp"
#include <assimp/mesh.h>
#include <glm/glm.hpp>
#include <vector>

struct MaterialVertexData {
  glm::vec3 position;
  glm::vec3 normal;
  glm::u32 material_idx;
};

struct TextureVertexData {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 tex_coords;
  glm::i32 texture_idx;
};

class Mesh {
public:
  Mesh(const aiMesh *mesh);
  GLsizei GetNumElements() const;
  GLsizei GetNumVertices() const;
  const std::vector<GLuint> GetElementBuffer(uint vertex_offset) const;
  const std::vector<MaterialVertexData>
  GetMaterialVertexBuffer(GLuint material_idx) const;
  const std::vector<TextureVertexData>
  GetTextureVertexBuffer(GLint texture_idx) const;

private:
  std::vector<GLfloat> m_vertices{};
  std::vector<GLfloat> m_normals{};
  std::vector<GLfloat> m_tex_coords{};
  std::vector<GLuint> m_faces{};
};