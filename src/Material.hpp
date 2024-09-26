#pragma once
#include <assimp/material.h>
#include <glm/glm.hpp>

struct BSDFMaterial {
  glm::vec3 ambient_color;
  float padding0;
  glm::vec3 diffuse_color;
  float padding1;
  glm::vec3 specular_color;
  float shininess;
};

class Material {
public:
  Material(const aiMaterial *material);
  const BSDFMaterial &GetProperties() const;
  std::string m_diffuse_texture{};

private:
  BSDFMaterial m_properties{};
};