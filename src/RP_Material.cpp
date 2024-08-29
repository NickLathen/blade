#include "RenderPass.hpp"
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

RP_Material::RP_Material()
    : mShader{"shaders/vertex.glsl", "shaders/fragment.glsl"} {
  mShader.setUniformBlockBinding("uMaterialBlock", muMaterialBlockBinding);
  glGenVertexArrays(1, &mVAO);
  glGenBuffers(1, &mVBO);
  glGenBuffers(1, &mEBO);
  glGenBuffers(1, &muMaterialUBO);
};

void RP_Material::init(const std::vector<Material> &materials,
                       const std::vector<MeshVertexBuffer> &vertexBufferData,
                       const std::vector<GLuint> &elementBufferData) {

  // Copy mMaterials to GPU muMaterialUBO
  uint numMaterials = materials.size();
  BSDFMaterial materialData[numMaterials];
  for (uint i = 0; i < numMaterials; i++) {
    materialData[i] = materials[i].getProperties();
  }
  glBindBuffer(GL_UNIFORM_BUFFER, muMaterialUBO);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(materialData), materialData,
               GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  // copy vertexBuffer/elementBuffer to GPU mVBO/mEBO
  glBindVertexArray(mVAO);
  mNumElements = elementBufferData.size();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               mNumElements * sizeof(elementBufferData[0]),
               &elementBufferData[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glBufferData(GL_ARRAY_BUFFER,
               vertexBufferData.size() * sizeof(vertexBufferData[0]),
               &vertexBufferData[0], GL_STATIC_DRAW);
  // configure vertex attributes
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexBuffer),
                        (GLvoid *)0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexBuffer),
                        (GLvoid *)(offsetof(MeshVertexBuffer, aNormal)));
  glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(MeshVertexBuffer),
                         (GLvoid *)(offsetof(MeshVertexBuffer, aMaterialIdx)));
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
};

void RP_Material::draw(const Camera &camera, const Light &light,
                       const ::glm::mat4 &uMVP, const glm::mat4 &uLightMVP,
                       GLuint FBO) {
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
  glm::vec3 uCameraPos = getCameraPos(camera.transform);
  glm::mat3 uWorldMatrix =
      glm::mat3(1.0f); // no inverse-transpose for orthogonal matrix
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
  glUniformMatrix3fv(mShader.getUniformLocation("uWorldMatrix"), 1, GL_FALSE,
                     glm::value_ptr(uWorldMatrix));
  glUniform1f(mShader.getUniformLocation("uSpecularPower"), 32.0f);
  glUniform1f(mShader.getUniformLocation("uShininessScale"), 2000.0f);

  // Bind Uniform Blocks
  glBindBufferBase(GL_UNIFORM_BUFFER, muMaterialBlockBinding, muMaterialUBO);

  // Bind Textures
  glUniform1i(mShader.getUniformLocation("uTexture"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, FBO);

  // Bind VAO
  glBindVertexArray(mVAO);

  // Draw
  glDrawElements(GL_TRIANGLES, mNumElements, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  if (gDepthTest == GL_FALSE)
    glDisable(GL_DEPTH_TEST);
  if (gCullFace == GL_FALSE)
    glDisable(GL_CULL_FACE);
  glCullFace(gCullFaceMode);
  glFrontFace(gFrontFace);
};

GLuint RP_Material::getVBO() const { return mVBO; };
GLuint RP_Material::getEBO() const { return mEBO; };
