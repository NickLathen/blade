#pragma once
#include "gl.hpp"
#include <assimp/mesh.h>
#include <vector>

struct MeshVertexBuffer {
  glm::vec3 aPos;
  glm::vec3 aNormal;
  glm::u32 aMaterialIdx;
};

class Mesh {
public:
  Mesh(const aiMesh *mesh);
  GLsizei getNumElements() const;
  GLsizei getNumVertices() const;
  const std::vector<GLuint> getElementBuffer(uint vertexOffset) const;
  const std::vector<MeshVertexBuffer> getVertexBuffer(GLuint materialIdx) const;

private:
  std::vector<GLfloat> mVertices{};
  std::vector<GLfloat> mNormals{};
  std::vector<GLuint> mFaces{};
};