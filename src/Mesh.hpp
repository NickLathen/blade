#pragma once
#include "gl.hpp"
#include <assimp/mesh.h>
#include <glm/glm.hpp>
#include <vector>

struct MeshVertexBuffer {
  glm::vec3 position;
  glm::vec3 normal;
  glm::u32 material_idx;
};

class Mesh {
public:
  Mesh(const aiMesh *mesh);
  GLsizei GetNumElements() const;
  GLsizei GetNumVertices() const;
  const std::vector<GLuint> GetElementBuffer(uint vertex_offset) const;
  const std::vector<MeshVertexBuffer>
  GetVertexBuffer(GLuint material_idx) const;

private:
  std::vector<GLfloat> m_vertices{};
  std::vector<GLfloat> m_normals{};
  std::vector<GLuint> m_faces{};
};