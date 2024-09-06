#include <assimp/Importer.hpp>
#include <iostream>

#include "MeshGroup.hpp"
#include "utils.hpp"

MeshGroup Import(const std::string &p_file) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(p_file, 0);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return nullptr;
  }
  return MeshGroup{scene};
}

MeshGroup::MeshGroup(const aiScene *scene) {
  // Construct m_materials
  for (uint i = 0; i < scene->mNumMaterials; i++) {
    AddMaterial(scene->mMaterials[i]);
  }

  // Construct m_meshes, m_mesh_map
  uint vertex_offset = 0;
  uint element_offset = 0;
  auto ProcessNode = [&](auto &ProcessNode, const aiNode *node) -> void {
    for (uint i = 0; i < node->mNumMeshes; i++) {
      const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      uint mesh_idx = AddMesh(mesh, vertex_offset, element_offset);
      element_offset += m_meshes[mesh_idx].GetNumElements();
      vertex_offset += m_meshes[mesh_idx].GetNumVertices();
    }
    for (uint i = 0; i < node->mNumChildren; i++) {
      ProcessNode(ProcessNode, node->mChildren[i]);
    }
  };
  ProcessNode(ProcessNode, scene->mRootNode);

  // Construct client m_vertex_buffer/m_element_buffer
  m_element_buffer.reserve(GetNumElements());
  m_vertex_buffer.reserve(GetNumVertices());
  for (uint mesh_idx = 0; mesh_idx < m_meshes.size(); mesh_idx++) {
    const Mesh &mesh = m_meshes[mesh_idx];
    const MeshMap &mesh_map = m_mesh_map[mesh_idx];
    const std::vector<GLuint> element_buffer =
        mesh.GetElementBuffer(mesh_map.vertex_offset);
    const std::vector<MeshVertexBuffer> vertex_buffer =
        mesh.GetVertexBuffer(mesh_map.material_idx);
    m_element_buffer.insert(m_element_buffer.end(), element_buffer.begin(),
                            element_buffer.end());
    m_vertex_buffer.insert(m_vertex_buffer.end(), vertex_buffer.begin(),
                           vertex_buffer.end());
  }
};

GLuint MeshGroup::GetNumElements() const { return m_element_buffer.size(); };
GLuint MeshGroup::GetNumVertices() const { return m_vertex_buffer.size(); };

const std::vector<Material> &MeshGroup::GetMaterials() const {
  return m_materials;
};
const std::vector<MeshVertexBuffer> &MeshGroup::GetVertexBuffer() const {
  return m_vertex_buffer;
};
const std::vector<GLuint> &MeshGroup::GetElementBuffer() const {
  return m_element_buffer;
};

uint MeshGroup::AddMaterial(const aiMaterial *material) {
  m_materials.emplace_back(material);
  return m_materials.size() - 1;
};

uint MeshGroup::AddMesh(const aiMesh *mesh, GLuint vertex_offset,
                        GLuint element_offset) {
  m_meshes.emplace_back(mesh);
  uint mesh_idx = m_meshes.size() - 1;
  m_mesh_map.push_back((MeshMap){.material_idx = mesh->mMaterialIndex,
                                 .vertex_offset = vertex_offset,
                                 .element_offset = element_offset});
  return mesh_idx;
}
