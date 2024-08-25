#pragma once
#include "gl.hpp"
#include <assimp/material.h>

struct BSDFMaterial {
  glm::vec3 ambientColor{1.0};
  float padding0{0.0};
  glm::vec3 diffuseColor{0.0};
  float padding1{0.0};
  glm::vec3 specularColor{0.0};
  float padding2{0.0};
  float shininess{0.0};
};

class Material {
public:
  Material(const aiMaterial *material);
  const BSDFMaterial &getProperties() const;

private:
  BSDFMaterial mProperties{};
};