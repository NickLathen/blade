#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <assimp/scene.h>
#include <vector>

struct MeshMap {
  GLuint materialId;
  GLuint vertexOffset;
  GLuint elementOffset;
};

class Actor {
public:
  Actor(const aiScene *scene);
  void draw(const glm::vec3 &cameraPos, float aspectRatio,
            const glm::mat4 &modelTransform);
  GLuint getNumElements() const;
  GLuint getNumVertices() const;

private:
  uint _addMaterial(const aiMaterial *material);
  uint _addMesh(const aiMesh *mesh, GLuint vertexOffset, GLuint elementOffset);
  std::vector<Mesh> mMeshes{};
  std::vector<MeshMap> mMeshMap{};
  std::vector<Material> mMaterials{};
  std::vector<GLuint> mElementBuffer{};
  std::vector<MeshVertexBuffer> mVertexBuffer{};
  Shader mShader;
  GLuint mVAO;
  GLuint mVBO;
  GLuint mEBO;
  GLuint muMaterialBlockBinding{0};
  GLuint muMaterialUBO{0};
};