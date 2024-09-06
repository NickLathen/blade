#include "utils.hpp"

#include <fstream>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <sstream>

std::string LoadFileIntoString(const std::string &file_path) {
  std::ifstream file_stream(file_path);
  if (!file_stream.is_open()) {
    throw std::runtime_error("Could not open file: " + file_path);
  }
  std::stringstream buffer;
  buffer << file_stream.rdbuf();
  return buffer.str();
}

void PrintMaterial(const aiMaterial *material) {
  printf("Material %s\n", material->GetName().C_Str());
  for (uint j = 0; j < material->mNumProperties; j++) {
    aiMaterialProperty *prop = material->mProperties[j];
    aiPropertyTypeInfo prop_type = prop->mType;

    const char *p_key = prop->mKey.C_Str();
    printf("%s ", p_key);
    switch (prop_type) {
    case aiPTI_Float: {
      float *float_out = (float *)prop->mData;
      uint size = prop->mDataLength / sizeof(float);
      for (uint k = 0; k < size; k++) {
        printf("%.2f", float_out[k]);
        if (k != size - 1) {
          printf(", ");
        }
      }
      break;
    };
    case aiPTI_Double: {
      double *double_out = (double *)prop->mData;
      uint size = prop->mDataLength / sizeof(double);
      for (uint k = 0; k < size; k++) {
        printf("%.2f", double_out[k]);
        if (k != size - 1) {
          printf(", ");
        }
      }
      break;
    }
    case aiPTI_String: {
      aiString string_out;
      material->Get(p_key, prop_type, 0, string_out);
      printf("%s", string_out.C_Str());
      break;
    }
    case aiPTI_Integer: {
      int *integer_out = (int *)prop->mData;
      uint size = prop->mDataLength / sizeof(int);
      for (uint k = 0; k < size; k++) {
        printf("%i", integer_out[k]);
        if (k != size - 1) {
          printf(", ");
        }
      }
      break;
    }
    case aiPTI_Buffer:
    case _aiPTI_Force32Bit:
      // no printable representation
      break;
    }
    printf("\n");
  }
}

void PrintVertices(const aiMesh *mesh) {
  for (uint j = 0; j < mesh->mNumVertices; j++) {
    aiVector3D vertex = mesh->mVertices[j];
    printf("v %.2f,%.2f,%.2f\n", vertex.x, vertex.y, vertex.z);
  }
}

void PrintNormals(const aiMesh *mesh) {
  if (mesh->mNormals) {
    for (uint j = 0; j < mesh->mNumVertices; j++) {
      aiVector3D vertex = mesh->mNormals[j];
      printf("vn %.2f,%.2f,%.2f\n", vertex.x, vertex.y, vertex.z);
    }
  }
}

void PrintTexCoords(const aiMesh *mesh) {
  for (uint j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; j++) {
    if (!mesh->mTextureCoords[j])
      continue;
    const aiString *tex_coords_name = mesh->GetTextureCoordsName(j);
    if (tex_coords_name) {
      printf("tex_coords %s\n", tex_coords_name->C_Str());
    } else {
      printf("unnamed tex_coords\n");
    }
    for (uint k = 0; k < mesh->mNumVertices; k++) {
      aiVector3D tex_coords = mesh->mTextureCoords[j][k];
      printf("vt %.2f,%.2f,%.2f\n", tex_coords.x, tex_coords.y, tex_coords.z);
    }
  }
}

void PrintFaces(const aiMesh *mesh) {
  for (uint j = 0; j < mesh->mNumFaces; j++) {
    aiFace face = mesh->mFaces[j];
    printf("f ");
    for (uint k = 0; k < face.mNumIndices; k++) {
      printf("%u", face.mIndices[k]);
      if (k != face.mNumIndices - 1) {
        printf("/");
      } else {
        printf("\n");
      }
    }
  }
}

void PrintNode(aiNode *node, const aiScene *scene) {
  printf("##Node##\n");
  printf("%u meshes.\n", node->mNumMeshes);
  printf("%u children.\n", node->mNumChildren);
  for (uint i = 0; i < node->mNumMeshes; i++) {
    printf("Mesh#%u\n", i);
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    PrintMaterial(scene->mMaterials[mesh->mMaterialIndex]);
    PrintVertices(mesh);
    PrintNormals(mesh);
    PrintTexCoords(mesh);
    PrintFaces(mesh);
  }
  for (uint i = 0; i < node->mNumChildren; i++) {
    PrintNode(node->mChildren[i], scene);
  }
}
void PrintMatrix(const glm::mat4 &m) {
  for (uint col = 0; col < 4; col++) {
    printf("%.2f, ", m[0][col]);
    printf("%.2f, ", m[1][col]);
    printf("%.2f, ", m[2][col]);
    printf("%.2f\n", m[3][col]);
  }
}

glm::vec3 GetTranslation(const glm::mat4 &transform) {
  return glm::vec3{transform[3][0], transform[3][1], transform[3][2]};
}
glm::vec3 GetXAxis(const glm::mat4 &transform) {
  return glm::normalize(
      glm::vec3(transform[0][0], transform[1][0], transform[2][0]));
}
glm::vec3 GetYAxis(const glm::mat4 &transform) {
  return glm::normalize(
      glm::vec3(transform[0][1], transform[1][1], transform[2][1]));
}
glm::vec3 GetZAxis(const glm::mat4 &transform) {
  return glm::normalize(
      glm::vec3(transform[0][2], transform[1][2], transform[2][2]));
}
glm::vec3 GetCameraRight(const glm::mat4 &camera_transform) {
  return GetXAxis(camera_transform);
}
glm::vec3 GetCameraUp(const glm::mat4 &camera_transform) {
  return GetYAxis(camera_transform);
}
glm::vec3 GetCameraForward(const glm::mat4 &camera_transform) {
  return -GetZAxis(camera_transform);
}

glm::vec3 GetCameraPos(const glm::mat4 &view_matrix) {
  glm::mat3 inverse_rotation{view_matrix};
  glm::mat3 rotation{glm::inverse(inverse_rotation)};
  glm::mat4 inverse_translation{glm::mat4(rotation) * view_matrix};
  glm::mat4 translation{glm::inverse(inverse_translation)};
  return GetTranslation(translation);
};

void OrbitYaw(glm::mat4 &view_matrix, glm::vec3 &target, float amount) {
  glm::mat4 translated = glm::translate(view_matrix, target);
  glm::mat4 rotated =
      glm::rotate(translated, -amount, glm::vec3(0.0f, 1.0f, 0.0f));
  view_matrix = glm::translate(rotated, -target);
};

void OrbitPitch(glm::mat4 &view_matrix, glm::vec3 &target, float amount) {
  glm::mat4 translated = glm::translate(view_matrix, target);
  glm::vec3 camera_right = GetCameraRight(view_matrix);
  glm::mat4 rotated = glm::rotate(translated, -amount, camera_right);
  view_matrix = glm::translate(rotated, -target);
};

void RotateYaw(glm::mat4 &view_matrix, float amount) {
  glm::vec3 view_position{GetCameraPos(view_matrix)};
  OrbitYaw(view_matrix, view_position, amount);
};
void RotatePitch(glm::mat4 &view_matrix, float amount) {
  glm::vec3 view_position{GetCameraPos(view_matrix)};
  OrbitPitch(view_matrix, view_position, amount);
};

void MoveAlongCameraAxes(glm::mat4 &view_matrix, const glm::vec3 &translation) {
  view_matrix[3][0] -= translation.x;
  view_matrix[3][1] -= translation.y;
  view_matrix[3][2] += translation.z;
}

void SlideViewWithTarget(glm::mat4 &view_matrix, glm::vec3 &target,
                         float amount_right, float amount_up) {
  glm::vec3 camera_right = GetCameraRight(view_matrix);
  glm::vec3 camera_up = GetCameraUp(view_matrix);
  glm::vec3 translation = camera_up * amount_up + camera_right * amount_right;
  MoveAlongCameraAxes(view_matrix, glm::vec3(amount_right, amount_up, 0.0));
  target += translation;
};
