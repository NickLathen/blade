#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <assimp/scene.h>
#include <vector>

struct MeshMap {
  GLuint VAO;
  GLuint VBO;
  GLuint EBO;
  GLuint materialId;
};

class Actor {
public:
  Actor(const aiScene *scene);
  void draw(const glm::vec3 &cameraPos, float aspectRatio,
            const glm::mat4 &modelTransform);

private:
  uint _addMaterial(const aiMaterial *material);
  uint _addMesh(const aiMesh *mesh);
  std::vector<Mesh> mMeshes{};
  std::vector<Material> mMaterials{};
  std::vector<MeshMap> mMeshMap{};
  Shader mShader;
  GLuint muMaterialBlockBinding{0};
  GLuint muMaterialUBO{0};
};