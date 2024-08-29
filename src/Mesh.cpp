#include "Mesh.hpp"
#include <glm/glm.hpp>

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
GLsizei Mesh::getNumVertices() const { return mVertices.size() / 3; }
const std::vector<GLuint> Mesh::getElementBuffer(uint vertexOffset) const {
  std::vector<GLuint> elementBuffer{mFaces};
  for (GLuint &idx : elementBuffer) {
    idx += vertexOffset;
  }
  return elementBuffer;
};
const std::vector<MeshVertexBuffer>
Mesh::getVertexBuffer(GLuint materialIdx) const {
  uint sz = getNumVertices();
  std::vector<MeshVertexBuffer> vertexBufferData{};
  vertexBufferData.reserve(getNumVertices());
  for (uint i = 0; i < sz; i++) {
    uint offset = i * 3;
    vertexBufferData.emplace_back((MeshVertexBuffer){
        .aPos = glm::vec3(mVertices[offset], mVertices[offset + 1],
                          mVertices[offset + 2]),
        .aNormal = glm::vec3(mNormals[offset], mNormals[offset + 1],
                             mNormals[offset + 2]),
        .aMaterialIdx = materialIdx});
  }
  return vertexBufferData;
};
