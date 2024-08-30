#include "RenderPass.hpp"
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RP_Material::RP_Material(const std::vector<Material> &materials,
                         const std::vector<MeshVertexBuffer> &vertexBufferData,
                         const std::vector<GLuint> &elementBufferData)
    : mShader{"shaders/vertex.glsl", "shaders/fragment.glsl"} {
  mShader.setUniformBlockBinding("uMaterialBlock", muMaterialBlockBinding);

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

  // Set Shader
  mShader.useProgram();
  // Set Uniforms
  glUniform3fv(mShader.getUniformLocation("uCameraPos"), 1,
               glm::value_ptr(uCameraPos));
  glUniform3fv(mShader.getUniformLocation("uAmbientLightColor"), 1,
               glm::value_ptr(light.uAmbientLightColor));
  glUniform3fv(mShader.getUniformLocation("uLightDir"), 1,
               glm::value_ptr(light.uLightDir));
  glUniform3fv(mShader.getUniformLocation("uLightPos"), 1,
               glm::value_ptr(light.uLightPos));
  glUniform3fv(mShader.getUniformLocation("uLightColor"), 1,
               glm::value_ptr(light.uLightColor));
  glUniformMatrix4fv(mShader.getUniformLocation("uMVP"), 1, GL_FALSE,
                     glm::value_ptr(uMVP));
  glUniformMatrix4fv(mShader.getUniformLocation("uLightMVP"), 1, GL_FALSE,
                     glm::value_ptr(uLightMVP));
  glUniformMatrix4fv(mShader.getUniformLocation("uModelMatrix"), 1, GL_FALSE,
                     glm::value_ptr(uModelMatrix));
  glUniform1f(mShader.getUniformLocation("uSpecularPower"), 32.0f);
  glUniform1f(mShader.getUniformLocation("uShininessScale"), 2000.0f);

  // Bind Uniform Blocks
  mUBO.bindBufferBase(muMaterialBlockBinding);

  // Bind Textures
  glUniform1i(mShader.getUniformLocation("uTexture"), 0);
  glActiveTexture(GL_TEXTURE0);
  FBO.bindTexture(GL_TEXTURE_2D);

  // Bind VAO
  mVAO.bindVertexArray();

  // Draw
  glDrawElements(GL_TRIANGLES, mNumElements, GL_UNSIGNED_INT, 0);

  mVAO.unbind();
  glBindTexture(GL_TEXTURE_2D, 0);

  if (gDepthTest == GL_FALSE)
    glDisable(GL_DEPTH_TEST);
  if (gCullFace == GL_FALSE)
    glDisable(GL_CULL_FACE);
  glCullFace(gCullFaceMode);
  glFrontFace(gFrontFace);
};

const RP_VBO &RP_Material::getVBO() const { return mVBO; };
const RP_EBO &RP_Material::getEBO() const { return mEBO; };
