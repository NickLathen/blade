#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <vector>

struct MeshMap {
  GLuint materialId;
  GLuint vertexOffset;
  GLuint elementOffset;
};

class MeshGroup {
public:
  MeshGroup(const aiScene *scene);
  GLuint getNumElements() const;
  GLuint getNumVertices() const;
  const std::vector<Material> &getMaterials() const;
  const std::vector<MeshVertexBuffer> &getVertexBuffer() const;
  const std::vector<GLuint> &getElementBuffer() const;

private:
  uint _addMaterial(const aiMaterial *material);
  uint _addMesh(const aiMesh *mesh, GLuint vertexOffset, GLuint elementOffset);
  std::vector<Mesh> mMeshes{};
  std::vector<MeshMap> mMeshMap{};
  std::vector<Material> mMaterials{};
  std::vector<GLuint> mElementBuffer{};
  std::vector<MeshVertexBuffer> mVertexBuffer{};
};