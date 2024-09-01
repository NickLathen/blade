#include "RenderPass.hpp"
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RP_Material::RP_Material(const std::vector<Material> &materials,
                         const std::vector<MeshVertexBuffer> &vertexBufferData,
                         const std::vector<GLuint> &elementBufferData)
    : mShader{"shaders/vertex.glsl", "shaders/fragment.glsl"} {
  mShader.useProgram();
  mShader.uniformBlockBlinding("uMaterialBlock", muMaterialBlockBinding);
  mShader.uniform1i("uLightDepthTexture", muLightDepthTexture);
  glUseProgram(0);

  // Copy mMaterials to GPU muMaterialUBO
  uint numMaterials = materials.size();
  BSDFMaterial materialData[numMaterials];
  for (uint i = 0; i < numMaterials; i++) {
    materialData[i] = materials[i].getProperties();
  }
  mUBO.bufferData(sizeof(materialData), materialData, GL_STATIC_DRAW);

  // copy vertexBuffer/elementBuffer to GPU mVBO/mEBO
  mNumElements = elementBufferData.size();
  mEBO.bufferData(mNumElements * sizeof(elementBufferData[0]),
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
      mVBO, 2, 1, GL_UNSIGNED_INT, sizeof(MeshVertexBuffer),
      (GLvoid *)(offsetof(MeshVertexBuffer, aMaterialIdx)));

  mEBO.bindBuffer();

  mVAO.unbind();
  mVBO.unbind();
  mEBO.unbind();
};

void RP_Material::draw(const glm::vec3 &uCameraPos, const Light &light,
                       const glm::mat4 &uMVP, const glm::mat4 &uLightMVP,
                       const glm::mat4 &uModelMatrix, const RP_FBO &FBO) const {
  // set globals
  GLboolean gDepthTest, gCullFace;
  GLint gCullFaceMode, gFrontFace;
  glGetBooleanv(GL_DEPTH_TEST, &gDepthTest);
  glGetBooleanv(GL_CULL_FACE, &gCullFace);
  glGetIntegerv(GL_CULL_FACE_MODE, &gCullFaceMode);
  glGetIntegerv(GL_FRONT_FACE, &gFrontFace);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  mShader.useProgram();
  mShader.uniform3fv("uCameraPos", uCameraPos);
  mShader.uniform3fv("uAmbientLightColor", light.uAmbientLightColor);
  mShader.uniform3fv("uLightDir", light.uLightDir);
  mShader.uniform3fv("uLightColor", light.uLightColor);
  mShader.uniform3fv("uLightPos", light.uLightPos);
  mShader.uniformMatrix4fv("uMVP", GL_FALSE, uMVP);
  mShader.uniformMatrix4fv("uLightMVP", GL_FALSE, uLightMVP);
  mShader.uniformMatrix4fv("uModelMatrix", GL_FALSE, uModelMatrix);
  mShader.uniform1f("uSpecularPower", 32.0f);
  mShader.uniform1f("uShininessScale", 2000.0f);

  mUBO.bindBufferBase(muMaterialBlockBinding);

  glActiveTexture(GL_TEXTURE0 + muLightDepthTexture);
  FBO.bindTexture(GL_TEXTURE_2D);

  drawVertices();

  glBindTexture(GL_TEXTURE_2D, 0);

  if (gDepthTest == GL_FALSE)
    glDisable(GL_DEPTH_TEST);
  if (gCullFace == GL_FALSE)
    glDisable(GL_CULL_FACE);
  glCullFace(gCullFaceMode);
  glFrontFace(gFrontFace);
};

void RP_Material::drawVertices() const {
  mVAO.bindVertexArray();
  glDrawElements(GL_TRIANGLES, mNumElements, GL_UNSIGNED_INT, 0);
  mVAO.unbind();
};

const RP_VBO &RP_Material::getVBO() const { return mVBO; };
const RP_EBO &RP_Material::getEBO() const { return mEBO; };
