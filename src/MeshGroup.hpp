#pragma once

#include <assimp/scene.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "ImageData.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "utils.hpp"

struct MeshMap {
  GLuint material_idx;
  int texture_idx{-1};
  GLuint vertex_offset;
  GLuint element_offset;
};

class MeshGroup {
public:
  MeshGroup(const aiScene *scene, const std::filesystem::path &parent_path);
  GLuint GetNumElements() const;
  const std::vector<Material> &GetMaterials() const;
  const std::vector<MaterialVertexData> &GetMaterialVertexBuffer() const;
  const std::vector<TextureVertexData> &GetTextureVertexBuffer() const;
  const std::vector<GLuint> &GetElementBuffer() const;
  const std::vector<ImageData> &GetTextureImages() const;
  const std::vector<MeshMap> &GetMeshMap() const;

private:
  uint AddMaterial(const aiMaterial *material);
  uint AddTexture(const std::string &path);
  uint AddMesh(const aiMesh *mesh, GLuint vertex_offset, GLuint element_offset,
               int texture_idx);
  std::vector<Mesh> m_meshes{};
  std::vector<MeshMap> m_mesh_map{};
  std::vector<Material> m_materials{};
  std::vector<ImageData> m_texture_images{};
  std::vector<GLuint> m_element_buffer{};
  std::vector<MaterialVertexData> m_material_vertex_buffer{};
  std::vector<TextureVertexData> m_texture_vertex_buffer{};
};

MeshGroup Import(const std::string &p_file);