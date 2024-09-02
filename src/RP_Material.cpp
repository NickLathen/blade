#include "RenderPass.hpp"
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RP_Material::RP_Material(const std::vector<Material> &materials,
                         const std::vector<MeshVertexBuffer> &vertexBufferData,
                         const std::vector<GLuint> &elementBufferData) {

  // Copy mMaterials to GPU muMaterialUBO
  uint numMaterials = materials.size();
  BSDFMaterial materialData[numMaterials];
  for (uint i = 0; i < numMaterials; i++) {
    materialData[i] = materials[i].getProperties();
  }
  mUBO.bufferData(sizeof(materialData), materialData, GL_STATIC_DRAW);

  // copy vertexBuffer/elementBuffer to GPU mVBO/mEBO
  mNumElements = elementBufferData.size();
  mEBO.bufferData(elementBufferData.size() * sizeof(elementBufferData[0]),
                  &elementBufferData[0], GL_STATIC_DRAW);
  mVBO.bufferData(vertexBufferData.size() * sizeof(vertexBufferData[0]),
                  &vertexBufferData[0], GL_STATIC_DRAW);

  mVAO.bindVertexArray();
  mVAO.vertexAttribPointer(mVBO, 0, 3, GL_FLOAT, GL_FALSE,
                           sizeof(MeshVertexBuffer), (GLvoid *)0);
  mVAO.vertexAttribPointer(mVBO, 1, 3, GL_FLOAT, GL_FALSE,
                           sizeof(MeshVertexBuffer),
                           (GLvoid *)(offsetof(MeshVertexBuffer, aNormal)));
  mVAO.vertexAttribIPointer(
      mVBO, 3, 1, GL_UNSIGNED_INT, sizeof(MeshVertexBuffer),
      (GLvoid *)(offsetof(MeshVertexBuffer, aMaterialIdx)));

  mEBO.bindBuffer();

  mVAO.unbind();
  mVBO.unbind();
  mEBO.unbind();
};

void RP_Material::drawVertices() const {
  mVAO.bindVertexArray();
  glDrawElements(GL_TRIANGLES, mNumElements, GL_UNSIGNED_INT, 0);
  mVAO.unbind();
};
