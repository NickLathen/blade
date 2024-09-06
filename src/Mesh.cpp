#include "Mesh.hpp"
#include <glm/glm.hpp>

Mesh::Mesh(const aiMesh *mesh)
    : m_vertices(mesh->mNumVertices * 3), m_normals(mesh->mNumVertices * 3),
      m_faces(mesh->mNumFaces * 3) {
  if (mesh->HasPositions()) {
    memcpy(&m_vertices[0], mesh->mVertices,
           m_vertices.size() * sizeof(m_vertices[0]));
  }
  if (mesh->HasNormals()) {
    memcpy(&m_normals[0], mesh->mNormals,
           m_normals.size() * sizeof(m_normals[0]));
  }
  if (mesh->HasFaces()) {
    for (uint i = 0; i < mesh->mNumFaces; i++) {
      uint offset = i * 3;
      memcpy(&m_faces[offset], mesh->mFaces[i].mIndices,
             sizeof(m_faces[0]) * 3);
    }
  }
};
GLsizei Mesh::GetNumElements() const { return m_faces.size(); }
GLsizei Mesh::GetNumVertices() const { return m_vertices.size() / 3; }
const std::vector<GLuint> Mesh::GetElementBuffer(uint vertex_offset) const {
  std::vector<GLuint> element_buffer{m_faces};
  for (GLuint &idx : element_buffer) {
    idx += vertex_offset;
  }
  return element_buffer;
};
const std::vector<MeshVertexBuffer>
Mesh::GetVertexBuffer(GLuint material_idx) const {
  uint sz = GetNumVertices();
  std::vector<MeshVertexBuffer> vertex_buffer_data{};
  vertex_buffer_data.reserve(GetNumVertices());
  for (uint i = 0; i < sz; i++) {
    uint offset = i * 3;
    vertex_buffer_data.emplace_back((MeshVertexBuffer){
        .position = glm::vec3(m_vertices[offset], m_vertices[offset + 1],
                              m_vertices[offset + 2]),
        .normal = glm::vec3(m_normals[offset], m_normals[offset + 1],
                            m_normals[offset + 2]),
        .material_idx = material_idx});
  }
  return vertex_buffer_data;
};
