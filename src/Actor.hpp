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
  glm::mat4 transform{1.0f};
  glm::vec3 target{};
  float aspectRatio{};
  float fov{};
  float near{};
  float far{};
};

struct Light {
  glm::vec3 uAmbientLightColor;
  glm::vec3 uLightDir;
  glm::vec3 uLightPos;
  glm::vec3 uLightColor;
};

class Actor {
public:
  Actor(const aiScene *scene);
  void draw(const Camera &camera, const Light &light, const ::glm::mat4 &uMVP,
            const glm::mat4 &uLightMVP, GLuint FBO);
  GLuint getNumElements() const;
  GLuint getNumVertices() const;
  GLuint mVAO;
  GLuint mVBO;
  GLuint mEBO;

private:
  uint _addMaterial(const aiMaterial *material);
  uint _addMesh(const aiMesh *mesh, GLuint vertexOffset, GLuint elementOffset);
  std::vector<Mesh> mMeshes{};
  std::vector<MeshMap> mMeshMap{};
  std::vector<Material> mMaterials{};
  std::vector<GLuint> mElementBuffer{};
  std::vector<MeshVertexBuffer> mVertexBuffer{};
  Shader mShader;
  GLuint muMaterialBlockBinding{0};
  GLuint muMaterialUBO{0};
};