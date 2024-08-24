#pragma once
#include "gl.hpp"

class Entity {
public:
  Entity(const glm::mat4 &transform);
  void update(const glm::mat4 &transform);
  const glm::mat4 &getTransform() const;

private:
  glm::mat4 mTransform{};
};