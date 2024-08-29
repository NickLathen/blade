#pragma once
#include <assimp/material.h>
#include <glm/glm.hpp>

struct BSDFMaterial {
  glm::vec3 ambientColor;
  float padding0;
  glm::vec3 diffuseColor;
  float padding1;
  glm::vec3 specularColor;
  float shininess;
};

class Material {
public:
  Material(const aiMaterial *material);
  const BSDFMaterial &getProperties() const;

private:
  BSDFMaterial mProperties{};
};