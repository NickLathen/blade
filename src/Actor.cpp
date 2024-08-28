#include "Actor.hpp"
#include "utils.hpp"

Actor::Actor(const aiScene *scene)
    : mShader{"shaders/vertex.glsl", "shaders/fragment.glsl"} {
  // init shader bindings
  mShader.setUniformBlockBinding("uMaterialBlock", muMaterialBlockBinding);

  // Construct mMaterials
  for (uint i = 0; i < scene->mNumMaterials; i++) {
    _addMaterial(scene->mMaterials[i]);
  }

  // Copy mMaterials to GPU muMaterialUBO
  uint numMaterials = mMaterials.size();
  BSDFMaterial materialData[numMaterials];
  for (uint i = 0; i < numMaterials; i++) {
    materialData[i] = mMaterials[i].getProperties();
  }
  glGenBuffers(1, &muMaterialUBO);
  glBindBuffer(GL_UNIFORM_BUFFER, muMaterialUBO);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(materialData), materialData,
               GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

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

  // Construct client vertexBuffer/elementBuffer
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

  // VAO - init
  glGenVertexArrays(1, &mVAO);
  glGenBuffers(1, &mVBO);
  glGenBuffers(1, &mEBO);

  // copy vertexBuffer/elementBuffer to GPU mVBO/mEBO
  glBindVertexArray(mVAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               mElementBuffer.size() * sizeof(mElementBuffer[0]),
               &mElementBuffer[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glBufferData(GL_ARRAY_BUFFER, mVertexBuffer.size() * sizeof(mVertexBuffer[0]),
               &mVertexBuffer[0], GL_STATIC_DRAW);
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
  // VAO - end
};

GLuint Actor::getNumElements() const { return mElementBuffer.size(); };
GLuint Actor::getNumVertices() const { return mVertexBuffer.size(); };

void Actor::draw(const Camera &camera, const Light &light, GLuint FBO) {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glEnable(GL_DEPTH_TEST);

  glBindVertexArray(mVAO);

  mShader.useProgram();

  glm::vec3 uCameraPos = getCameraPos(camera.transform);
  glm::mat4 projection = glm::perspective(
      glm::radians(camera.fov), camera.aspectRatio, camera.near, camera.far);
  glm::mat4 mvp = projection * camera.transform;
  glm::mat4 lightTransform =
      glm::lookAt(light.uLightPos, camera.target, glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 lightProjection = glm::perspective(
      glm::radians(90.0f), camera.aspectRatio, camera.near, camera.far);
  glm::mat4 uLightMVP = lightProjection * lightTransform;

  glm::mat3 uWorldMatrix =
      glm::mat3(1.0f); // no inverse-transpose for orthogonal matrix

  glUniform1i(mShader.getUniformLocation("uTexture"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, FBO);

  // Set Per Actor Uniforms
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
                     glm::value_ptr(mvp));
  glUniformMatrix4fv(mShader.getUniformLocation("uLightMVP"), 1, GL_FALSE,
                     glm::value_ptr(uLightMVP));
  glUniformMatrix3fv(mShader.getUniformLocation("uWorldMatrix"), 1, GL_FALSE,
                     glm::value_ptr(uWorldMatrix));
  glUniform1f(mShader.getUniformLocation("uSpecularPower"), 32.0f);
  glUniform1f(mShader.getUniformLocation("uShininessScale"), 2000.0f);

  // Bind Per Actor Uniform Blocks
  glBindBufferBase(GL_UNIFORM_BUFFER, muMaterialBlockBinding, muMaterialUBO);

  glDrawElements(GL_TRIANGLES, getNumElements(), GL_UNSIGNED_INT, 0);
  // if we want to render individual meshes
  // for (uint i = 0; i < mMeshes.size(); i++) {
  //   glDrawElements(
  //       GL_TRIANGLES, mMeshes[i].getNumElements(), GL_UNSIGNED_INT,
  //       (void *)(mMeshMap[i].elementOffset * sizeof(mElementBuffer[0])));
  // }
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

uint Actor::_addMaterial(const aiMaterial *material) {
  mMaterials.emplace_back(material);
  return mMaterials.size() - 1;
};

uint Actor::_addMesh(const aiMesh *mesh, GLuint vertexOffset,
                     GLuint elementOffset) {
  mMeshes.emplace_back(mesh);
  uint meshIdx = mMeshes.size() - 1;
  mMeshMap.push_back((MeshMap){.materialId = mesh->mMaterialIndex,
                               .vertexOffset = vertexOffset,
                               .elementOffset = elementOffset});
  return meshIdx;
}
