#pragma once
#include "gl.hpp"
#include <assimp/mesh.h>
#include <vector>

class Mesh {
public:
  Mesh(const aiMesh *mesh);
  GLsizei getNumElements() const;
  void packVAO(GLuint VAO, GLuint VBO, GLuint EBO, GLuint materialIdx) const;

private:
  std::vector<GLfloat> mVertices{};
  std::vector<GLfloat> mNormals{};
  std::vector<GLuint> mFaces{};
};