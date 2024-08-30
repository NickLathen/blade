#include <assimp/Importer.hpp>
#include <iostream>

#include "MeshGroup.hpp"
#include "utils.hpp"

MeshGroup import(const std::string &pFile) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(pFile, 0);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return nullptr;
  }
  return MeshGroup{scene};
}

MeshGroup::MeshGroup(const aiScene *scene) {
  // Construct mMaterials
  for (uint i = 0; i < scene->mNumMaterials; i++) {
    _addMaterial(scene->mMaterials[i]);
  }

  // Construct mMeshes, mMeshMap
  uint vertexOffset = 0;
  uint elementOffset = 0;
  auto processNode = [&](auto &processNode, const aiNode *node) -> void {
    for (uint i = 0; i < node->mNumMeshes; i++) {
      const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      uint meshIdx = _addMesh(mesh, vertexOffset, elementOffset);
      elementOffset += mMeshes[meshIdx].getNumElements();
      vertexOffset += mMeshes[meshIdx].getNumVertices();
    }
    for (uint i = 0; i < node->mNumChildren; i++) {
      processNode(processNode, node->mChildren[i]);
    }
  };
  processNode(processNode, scene->mRootNode);

  // Construct client mVertexBuffer/mElementBuffer
  mElementBuffer.reserve(getNumElements());
  mVertexBuffer.reserve(getNumVertices());
  for (uint meshIdx = 0; meshIdx < mMeshes.size(); meshIdx++) {
    const Mesh &mesh = mMeshes[meshIdx];
    const MeshMap &meshMap = mMeshMap[meshIdx];
    const std::vector<GLuint> elementBuffer =
        mesh.getElementBuffer(meshMap.vertexOffset);
    const std::vector<MeshVertexBuffer> vertexBuffer =
        mesh.getVertexBuffer(meshMap.materialId);
    mElementBuffer.insert(mElementBuffer.end(), elementBuffer.begin(),
                          elementBuffer.end());
    mVertexBuffer.insert(mVertexBuffer.end(), vertexBuffer.begin(),
                         vertexBuffer.end());
  }
};

GLuint MeshGroup::getNumElements() const { return mElementBuffer.size(); };
GLuint MeshGroup::getNumVertices() const { return mVertexBuffer.size(); };

const std::vector<Material> &MeshGroup::getMaterials() const {
  return mMaterials;
};
const std::vector<MeshVertexBuffer> &MeshGroup::getVertexBuffer() const {
  return mVertexBuffer;
};
const std::vector<GLuint> &MeshGroup::getElementBuffer() const {
  return mElementBuffer;
};

uint MeshGroup::_addMaterial(const aiMaterial *material) {
  mMaterials.emplace_back(material);
  return mMaterials.size() - 1;
};

uint MeshGroup::_addMesh(const aiMesh *mesh, GLuint vertexOffset,
                         GLuint elementOffset) {
  mMeshes.emplace_back(mesh);
  uint meshIdx = mMeshes.size() - 1;
  mMeshMap.push_back((MeshMap){.materialId = mesh->mMaterialIndex,
                               .vertexOffset = vertexOffset,
                               .elementOffset = elementOffset});
  return meshIdx;
}
