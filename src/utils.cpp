#include "utils.hpp"

#include <fstream>
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