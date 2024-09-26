#include <assimp/Importer.hpp>
#include <filesystem>
#include <iostream>
#include <unordered_map>

#include "MeshGroup.hpp"
#include "utils.hpp"

MeshGroup Import(const std::string &filename) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(filename, 0);
  std::filesystem::path fs_path{filename};

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    exit(1);
  }
  return MeshGroup{scene, fs_path.parent_path()};
}

MeshGroup::MeshGroup(const aiScene *scene,
                     const std::filesystem::path &parent_path) {
  // Construct m_materials
  for (uint i = 0; i < scene->mNumMaterials; i++) {
    AddMaterial(scene->mMaterials[i]);
  }
  // Construct m_texture_images
  std::unordered_map<uint, uint> materialTextureMap{};
  for (uint i = 0; i < m_materials.size(); i++) {
    const Material &material = m_materials[i];
    if (material.m_diffuse_texture.size() > 0) {
      std::filesystem::path p{parent_path / material.m_diffuse_texture};
      uint textureIdx = AddTexture(p.lexically_normal());
      // map materialIdx to textureIdx
      materialTextureMap.insert({i, textureIdx});
    }
  }
  // Construct m_meshes, m_mesh_map
  //   Traverse tree and extract array of meshes
  std::vector<const aiMesh *> meshes{};
  auto ProcessNode = [&](auto &ProcessNode, const aiNode *node) -> void {
    for (uint i = 0; i < node->mNumMeshes; i++) {
      meshes.push_back(scene->mMeshes[node->mMeshes[i]]);
    }
    for (uint i = 0; i < node->mNumChildren; i++) {
      ProcessNode(ProcessNode, node->mChildren[i]);
    }
  };
  ProcessNode(ProcessNode, scene->mRootNode);
  //  Sort meshes according to texture/material id
  std::sort(meshes.begin(), meshes.end(), [](const aiMesh *a, const aiMesh *b) {
    return a->mMaterialIndex < b->mMaterialIndex;
  });
  //  Iterate through meshes and append to m_meshes
  uint vertex_offset = 0;
  uint element_offset = 0;
  for (uint i = 0; i < meshes.size(); i++) {
    const aiMesh *mesh = meshes[i];
    int texture_idx = -1;
    if (materialTextureMap.find(mesh->mMaterialIndex) !=
        materialTextureMap.end()) {
      texture_idx = materialTextureMap.at(mesh->mMaterialIndex);
    }
    uint mesh_idx = AddMesh(mesh, vertex_offset, element_offset, texture_idx);
    element_offset += m_meshes[mesh_idx].GetNumElements();
    vertex_offset += m_meshes[mesh_idx].GetNumVertices();
  }

  // Construct client m_vertex_buffer/m_element_buffer
  for (uint mesh_idx = 0; mesh_idx < m_meshes.size(); mesh_idx++) {
    const Mesh &mesh = m_meshes[mesh_idx];
    const MeshMap &mesh_map = m_mesh_map[mesh_idx];
    const std::vector<GLuint> element_buffer =
        mesh.GetElementBuffer(mesh_map.vertex_offset);
    m_element_buffer.insert(m_element_buffer.end(), element_buffer.begin(),
                            element_buffer.end());
    if (mesh_map.texture_idx >= 0) {
      const std::vector<TextureVertexData> vertex_buffer =
          mesh.GetTextureVertexBuffer(mesh_map.texture_idx);
      m_texture_vertex_buffer.insert(m_texture_vertex_buffer.end(),
                                     vertex_buffer.begin(),
                                     vertex_buffer.end());
    } else {
      const std::vector<MaterialVertexData> vertex_buffer =
          mesh.GetMaterialVertexBuffer(mesh_map.material_idx);
      m_material_vertex_buffer.insert(m_material_vertex_buffer.end(),
                                      vertex_buffer.begin(),
                                      vertex_buffer.end());
    }
  }
};

GLuint MeshGroup::GetNumElements() const { return m_element_buffer.size(); };

const std::vector<Material> &MeshGroup::GetMaterials() const {
  return m_materials;
};

const std::vector<MeshMap> &MeshGroup::GetMeshMap() const {
  return m_mesh_map;
};

const std::vector<MaterialVertexData> &
MeshGroup::GetMaterialVertexBuffer() const {
  return m_material_vertex_buffer;
};

const std::vector<TextureVertexData> &
MeshGroup::GetTextureVertexBuffer() const {
  return m_texture_vertex_buffer;
};

const std::vector<GLuint> &MeshGroup::GetElementBuffer() const {
  return m_element_buffer;
};

const std::vector<ImageData> &MeshGroup::GetTextureImages() const {
  return m_texture_images;
}

uint MeshGroup::AddMaterial(const aiMaterial *material) {
  m_materials.emplace_back(material);
  return m_materials.size() - 1;
};

uint MeshGroup::AddTexture(const std::string &path) {
  m_texture_images.emplace_back(path, 0);
  return m_texture_images.size() - 1;
};

uint MeshGroup::AddMesh(const aiMesh *mesh, GLuint vertex_offset,
                        GLuint element_offset, int texture_idx) {
  m_meshes.emplace_back(mesh);
  uint mesh_idx = m_meshes.size() - 1;
  m_mesh_map.push_back((MeshMap){.material_idx = mesh->mMaterialIndex,
                                 .texture_idx = texture_idx,
                                 .vertex_offset = vertex_offset,
                                 .element_offset = element_offset});
  return mesh_idx;
}
