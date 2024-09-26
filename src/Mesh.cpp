#include "Mesh.hpp"
#include <glm/glm.hpp>

Mesh::Mesh(const aiMesh *mesh)
    : m_vertices(mesh->mNumVertices * 3), m_normals(mesh->mNumVertices * 3),
      m_tex_coords(mesh->mNumVertices * 2), m_faces(mesh->mNumFaces * 3) {
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
  if (mesh->HasTextureCoords(0)) {
    for (uint i = 0; i < mesh->mNumVertices; i++) {
      uint offset = i * 2;
      memcpy(&m_tex_coords[offset], &mesh->mTextureCoords[0][i],
             sizeof(m_tex_coords[0]) * 2);
    }
  }
};
GLsizei Mesh::GetNumElements() const { return m_faces.size(); };
GLsizei Mesh::GetNumVertices() const { return m_vertices.size() / 3; };
const std::vector<GLuint> Mesh::GetElementBuffer(uint vertex_offset) const {
  std::vector<GLuint> element_buffer{m_faces};
  for (GLuint &idx : element_buffer) {
    idx += vertex_offset;
  }
  return element_buffer;
};
const std::vector<MaterialVertexData>
Mesh::GetMaterialVertexBuffer(GLuint material_idx) const {
  uint sz = GetNumVertices();
  std::vector<MaterialVertexData> vertex_buffer_data{};
  vertex_buffer_data.reserve(sz);
  for (uint i = 0; i < sz; i++) {
    uint offset = i * 3;
    vertex_buffer_data.emplace_back((MaterialVertexData){
        .position = glm::vec3(m_vertices[offset], m_vertices[offset + 1],
                              m_vertices[offset + 2]),
        .normal = glm::vec3(m_normals[offset], m_normals[offset + 1],
                            m_normals[offset + 2]),
        .material_idx = material_idx});
  }
  return vertex_buffer_data;
};

const std::vector<TextureVertexData>
Mesh::GetTextureVertexBuffer(GLint texture_idx) const {
  uint sz = GetNumVertices();
  std::vector<TextureVertexData> vertex_buffer_data{};
  vertex_buffer_data.reserve(sz);
  for (uint i = 0; i < sz; i++) {
    uint offset = i * 3;
    uint texOffset = i * 2;
    vertex_buffer_data.emplace_back((TextureVertexData){
        .position = glm::vec3(m_vertices[offset], m_vertices[offset + 1],
                              m_vertices[offset + 2]),
        .normal = glm::vec3(m_normals[offset], m_normals[offset + 1],
                            m_normals[offset + 2]),
        .tex_coords =
            glm::vec2(m_tex_coords[texOffset], m_tex_coords[texOffset + 1]),
        .texture_idx = texture_idx});
  }
  return vertex_buffer_data;
};
