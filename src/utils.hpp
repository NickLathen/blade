#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <string>

#define NEVER_COPY(T)                                                          \
  T(const T &) = delete;                                                       \
  T &operator=(const T &) = delete;

std::string LoadFileIntoString(const std::string &file_path);
void PrintMaterial(const aiMaterial *material);
void PrintVertices(const aiMesh *mesh);
void PrintNormals(const aiMesh *mesh);
void PrintTexCoords(const aiMesh *mesh);
void PrintFaces(const aiMesh *mesh);
void PrintNode(aiNode *node, const aiScene *scene);
void PrintMatrix(const glm::mat4 &m);
glm::vec3 GetCameraPos(const glm::mat4 &view_matrix);
void ZoomCamera(glm::mat4 &view_matrix, glm::vec3 &target, float amount);
void OrbitYaw(glm::mat4 &view_matrix, glm::vec3 &target, float amount);
void OrbitPitch(glm::mat4 &view_matrix, glm::vec3 &target, float amount);
void RotateYaw(glm::mat4 &view_matrix, float amount);
void RotatePitch(glm::mat4 &view_matrix, float amount);
void SlideViewWithTarget(glm::mat4 &view_matrix, glm::vec3 &target,
                         float amount_right, float amount_up);
void MoveAlongCameraAxes(glm::mat4 &view_matrix, const glm::vec3 &translation);