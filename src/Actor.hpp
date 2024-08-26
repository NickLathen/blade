#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <vector>

struct MeshMap {
  GLuint materialId;
  GLuint vertexOffset;
  GLuint elementOffset;
};

struct Camera {
  glm::vec3 up{0.0, 1.0, 0.0};
  glm::vec3 position{0};
  glm::vec3 target{0};
  float aspectRatio{};
  float fov{};
  float near{};
  float far{};
};

class Actor {
public:
  Actor(const aiScene *scene);
  void draw(const Camera &camera, const glm::mat4 &actorTransform);
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