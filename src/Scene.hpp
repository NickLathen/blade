#pragma once
#include "Entity.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <assimp/scene.h>
#include <vector>

class Scene {
public:
  Scene(const aiScene *scene);
  void draw(const Shader &shader, const glm::mat4 &viewProjection);

private:
  uint _addEntity(const aiMesh *mesh, uint materialIdx,
                  const glm::mat4 &transform);
  uint _addMaterial(const aiMaterial *material);
  uint _addMesh(const aiMesh *mesh);
  std::vector<Entity> mEntities{};
  std::vector<Mesh> mMeshes{};
  std::vector<Material> mMaterials{};
  std::vector<uint> mMap_entity_mesh{};
  std::vector<uint> mMap_entity_material{};
  std::vector<GLuint> mMap_mesh_vao{};
  std::vector<GLuint> mMap_mesh_vbo{};
  std::vector<GLuint> mMap_mesh_ebo{};
};