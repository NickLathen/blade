#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <string>

std::string loadFileIntoString(const std::string &filePath);
void printMaterial(const aiMaterial *material);
void printVertices(const aiMesh *mesh);
void printNormals(const aiMesh *mesh);
void printTexCoords(const aiMesh *mesh);
void printFaces(const aiMesh *mesh);
void printNode(aiNode *node, const aiScene *scene);
void printMatrix(const glm::mat4 &m);
glm::vec3 getCameraPos(const glm::mat4 &viewMatrix);