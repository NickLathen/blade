#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Material.hpp"
#include "Mesh.hpp"
#include "utils.hpp"

struct MeshMap {
  GLuint material_idx;
  GLuint vertex_offset;
  GLuint element_offset;
};

class MeshGroup {
public:
  MeshGroup(const aiScene *scene);
  GLuint GetNumElements() const;
  GLuint GetNumVertices() const;
  const std::vector<Material> &GetMaterials() const;
  const std::vector<MeshVertexBuffer> &GetVertexBuffer() const;
  const std::vector<GLuint> &GetElementBuffer() const;

private:
  uint AddMaterial(const aiMaterial *material);
  uint AddMesh(const aiMesh *mesh, GLuint vertex_offset, GLuint element_offset);
  std::vector<Mesh> m_meshes{};
  std::vector<MeshMap> m_mesh_map{};
  std::vector<Material> m_materials{};
  std::vector<GLuint> m_element_buffer{};
  std::vector<MeshVertexBuffer> m_vertex_buffer{};
};

MeshGroup Import(const std::string &p_file);