#pragma once
#include "gl.hpp"
#include <assimp/material.h>

class Material {
public:
  Material(const aiMaterial *material);
  const glm::vec3 &getDiffuse() const;

private:
  glm::vec3 mDiffuse{0};
};