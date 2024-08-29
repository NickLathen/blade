#include "utils.hpp"

#include <fstream>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <sstream>

std::string loadFileIntoString(const std::string &filePath) {
  std::ifstream fileStream(filePath);
  if (!fileStream.is_open()) {
    throw std::runtime_error("Could not open file: " + filePath);
  }
  std::stringstream buffer;
  buffer << fileStream.rdbuf();
  return buffer.str();
}

void printMaterial(const aiMaterial *material) {
  printf("Material %s\n", material->GetName().C_Str());
  for (uint j = 0; j < material->mNumProperties; j++) {
    aiMaterialProperty *prop = material->mProperties[j];
    aiPropertyTypeInfo propType = prop->mType;

    const char *pKey = prop->mKey.C_Str();
    printf("%s ", pKey);
    switch (propType) {
    case aiPTI_Float: {
      float *fOut = (float *)prop->mData;
      uint size = prop->mDataLength / sizeof(float);
      for (uint k = 0; k < size; k++) {
        printf("%.2f", fOut[k]);
        if (k != size - 1) {
          printf(", ");
        }
      }
      break;
    };
    case aiPTI_Double: {
      double *dOut = (double *)prop->mData;
      uint size = prop->mDataLength / sizeof(double);
      for (uint k = 0; k < size; k++) {
        printf("%.2f", dOut[k]);
        if (k != size - 1) {
          printf(", ");
        }
      }
      break;
    }
    case aiPTI_String: {
      aiString sOut;
      material->Get(pKey, propType, 0, sOut);
      printf("%s", sOut.C_Str());
      break;
    }
    case aiPTI_Integer: {
      int *iOut = (int *)prop->mData;
      uint size = prop->mDataLength / sizeof(int);
      for (uint k = 0; k < size; k++) {
        printf("%i", iOut[k]);
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

void printVertices(const aiMesh *mesh) {
  for (uint j = 0; j < mesh->mNumVertices; j++) {
    aiVector3D vertex = mesh->mVertices[j];
    printf("v %.2f,%.2f,%.2f\n", vertex.x, vertex.y, vertex.z);
  }
}

void printNormals(const aiMesh *mesh) {
  if (mesh->mNormals) {
    for (uint j = 0; j < mesh->mNumVertices; j++) {
      aiVector3D vertex = mesh->mNormals[j];
      printf("vn %.2f,%.2f,%.2f\n", vertex.x, vertex.y, vertex.z);
    }
  }
}

void printTexCoords(const aiMesh *mesh) {
  for (uint j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; j++) {
    if (!mesh->mTextureCoords[j])
      continue;
    const aiString *texCoordsName = mesh->GetTextureCoordsName(j);
    if (texCoordsName) {
      printf("texCoords %s\n", texCoordsName->C_Str());
    } else {
      printf("unnamed texCoords\n");
    }
    for (uint k = 0; k < mesh->mNumVertices; k++) {
      aiVector3D texCoords = mesh->mTextureCoords[j][k];
      printf("vt %.2f,%.2f,%.2f\n", texCoords.x, texCoords.y, texCoords.z);
    }
  }
}

void printFaces(const aiMesh *mesh) {
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

void printNode(aiNode *node, const aiScene *scene) {
  printf("##Node##\n");
  printf("%u meshes.\n", node->mNumMeshes);
  printf("%u children.\n", node->mNumChildren);
  for (uint i = 0; i < node->mNumMeshes; i++) {
    printf("Mesh#%u\n", i);
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    printMaterial(scene->mMaterials[mesh->mMaterialIndex]);
    printVertices(mesh);
    printNormals(mesh);
    printTexCoords(mesh);
    printFaces(mesh);
  }
  for (uint i = 0; i < node->mNumChildren; i++) {
    printNode(node->mChildren[i], scene);
  }
}
void printMatrix(const glm::mat4 &m) {
  for (uint col = 0; col < 4; col++) {
    printf("%.2f, ", m[col][0]);
    printf("%.2f, ", m[col][1]);
    printf("%.2f, ", m[col][2]);
    printf("%.2f\n", m[col][3]);
  }
}

glm::vec3 getCameraPos(const glm::mat4 &viewMatrix) {
  glm::mat3 inverseRotation{viewMatrix};
  glm::mat3 rotation{glm::inverse(inverseRotation)};
  glm::mat4 inverseTranslation{glm::mat4(rotation) * viewMatrix};
  glm::mat4 translation{glm::inverse(inverseTranslation)};
  return glm::vec3{translation[3][0], translation[3][1], translation[3][2]};
};

void zoomCamera(glm::mat4 &viewMatrix, glm::vec3 &target, float zoomAmount) {
  glm::vec3 cameraForward =
      glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
  cameraForward = glm::normalize(cameraForward);
  // prevent zooming through target
  viewMatrix = glm::translate(viewMatrix, cameraForward * zoomAmount);
}

void orbitYaw(glm::mat4 &viewMatrix, glm::vec3 &target, float amount) {
  glm::mat4 translated = glm::translate(viewMatrix, target);
  glm::mat4 rotated =
      glm::rotate(translated, amount * 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
  viewMatrix = glm::translate(rotated, -target);
};

void orbitPitch(glm::mat4 &viewMatrix, glm::vec3 &target, float amount) {
  glm::mat4 translated = glm::translate(viewMatrix, target);
  glm::vec3 cameraRight =
      glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
  cameraRight = glm::normalize(cameraRight);
  glm::mat4 rotated = glm::rotate(translated, amount * 0.01f, cameraRight);
  viewMatrix = glm::translate(rotated, -target);
};

void slideView(glm::mat4 &viewMatrix, glm::vec3 &target, float xAmount,
               float yAmount) {
  glm::vec3 cameraRight =
      glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
  cameraRight = glm::normalize(cameraRight);
  glm::vec3 cameraUp =
      glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
  cameraUp = glm::normalize(cameraUp);
  glm::vec3 translation =
      cameraUp * yAmount * 0.01f + cameraRight * -xAmount * 0.01f;
  viewMatrix = glm::translate(viewMatrix, translation);
  target -= translation;
};
