#include "Entity.hpp"

Entity::Entity(const glm::mat4 &transform) : mTransform{transform} {};

void Entity::update(const glm::mat4 &transform) { mTransform = transform; };

const glm::mat4 &Entity::getTransform() const { return mTransform; }