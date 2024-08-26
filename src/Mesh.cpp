#include "Mesh.hpp"

Mesh::Mesh(const aiMesh *mesh)
    : mVertices(mesh->mNumVertices * 3), mNormals(mesh->mNumVertices * 3),
      mFaces(mesh->mNumFaces * 3) {
  if (mesh->HasPositions()) {
    memcpy(&mVertices[0], mesh->mVertices,
           mVertices.size() * sizeof(mVertices[0]));
  }
  if (mesh->HasNormals()) {
    memcpy(&mNormals[0], mesh->mNormals, mNormals.size() * sizeof(mNormals[0]));
  }
  if (mesh->HasFaces()) {
    for (uint i = 0; i < mesh->mNumFaces; i++) {
      uint offset = i * 3;
      memcpy(&mFaces[offset], mesh->mFaces[i].mIndices, sizeof(mFaces[0]) * 3);
    }
  }
};
GLsizei Mesh::getNumElements() const { return mFaces.size(); }
void Mesh::packVAO(GLuint VAO, GLuint VBO, GLuint EBO,
                   GLuint materialIdx) const {
  struct VBuff {
    glm::vec3 aPos;
    glm::vec3 aNormal;
    glm::u32 aMaterialIdx;
  };
  uint sz = mVertices.size() / 3;
  VBuff bufferData[sz];
  for (uint i = 0; i < sz; i++) {
    uint offset = i * 3;
    memcpy(&bufferData[i], &mVertices[offset], sizeof(glm::vec3));
    memcpy(&bufferData[i].aNormal, &mNormals[offset], sizeof(glm::vec3));
    bufferData[i].aMaterialIdx = materialIdx;
  }
  glBindVertexArray(VAO);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(bufferData), bufferData, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VBuff), (GLvoid *)0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VBuff),
                        (GLvoid *)(offsetof(VBuff, aNormal)));
  glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(VBuff),
                         (GLvoid *)(offsetof(VBuff, aMaterialIdx)));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mFaces.size(),
               &mFaces[0], GL_STATIC_DRAW);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
