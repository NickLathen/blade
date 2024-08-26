#include "Actor.hpp"

Actor::Actor(const aiScene *scene)
    : mShader{"shaders/vertex.glsl", "shaders/fragment.glsl"} {
  mShader.setUniformBlockBinding("uMaterialBlock", muMaterialBlockBinding);

  auto processNode = [&](auto &processNode, const aiNode *node) -> void {
    for (uint i = 0; i < node->mNumMeshes; i++) {
      const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      _addMesh(mesh);
    }
    for (uint i = 0; i < node->mNumChildren; i++) {
      processNode(processNode, node->mChildren[i]);
    }
  };
  for (uint i = 0; i < scene->mNumMaterials; i++) {
    _addMaterial(scene->mMaterials[i]);
  }
  processNode(processNode, scene->mRootNode);

  // fill material buffer
  uint numMaterials = mMaterials.size();
  BSDFMaterial materialData[numMaterials];
  for (uint i = 0; i < numMaterials; i++) {
    materialData[i] = mMaterials[i].getProperties();
  }
  glGenBuffers(1, &muMaterialUBO);
  glBindBuffer(GL_UNIFORM_BUFFER, muMaterialUBO);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(materialData), materialData,
               GL_STATIC_DRAW);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
};

void Actor::draw(const glm::vec3 &cameraPos, float aspectRatio,
                 const glm::mat4 &actorTransform) {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glEnable(GL_DEPTH_TEST);

  mShader.useProgram();

  glm::vec3 uAmbientLightColor = glm::vec3{.3, .3, .3};
  glm::vec3 uLightDir = glm::vec3(-1, -5, 1);
  glm::vec3 uLightColor = glm::vec3{1, 1, 1};

  glm::vec3 up{0.0f, 1.0f, 0.0f};
  glm::vec3 cameraTargetPos{0.0f, 0.5f, 0.0f};
  glm::mat4 view = glm::lookAt(cameraPos, cameraTargetPos, up);
  glm::mat4 projection =
      glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
  glm::mat4 mvp = projection * view * actorTransform;

  glm::mat3 uWorldMatrix =
      glm::mat3(actorTransform); // no inverse-transpose for orthogonal matrix

  // Set Per Actor Uniforms
  glUniform3fv(mShader.getUniformLocation("uCameraPos"), 1,
               glm::value_ptr(cameraPos));
  glUniform3fv(mShader.getUniformLocation("uAmbientLightColor"), 1,
               glm::value_ptr(uAmbientLightColor));
  glUniform3fv(mShader.getUniformLocation("uLightDir"), 1,
               glm::value_ptr(uLightDir));
  glUniform3fv(mShader.getUniformLocation("uLightColor"), 1,
               glm::value_ptr(uLightColor));
  glUniformMatrix4fv(mShader.getUniformLocation("uMVP"), 1, GL_FALSE,
                     glm::value_ptr(mvp));
  glUniformMatrix3fv(mShader.getUniformLocation("uWorldMatrix"), 1, GL_FALSE,
                     glm::value_ptr(uWorldMatrix));

  // Bind Per Actor Uniform Blocks
  glBindBufferBase(GL_UNIFORM_BUFFER, muMaterialBlockBinding, muMaterialUBO);

  // Render Each Mesh
  for (uint i = 0; i < mMeshes.size(); i++) {
    const Mesh &mesh = mMeshes[i];
    const MeshMap &meshMap = mMeshMap[i];
    glBindVertexArray(meshMap.VAO);
    glDrawElements(GL_TRIANGLES, mesh.getNumElements(), GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);
}

uint Actor::_addMaterial(const aiMaterial *material) {
  mMaterials.emplace_back(material);
  return mMaterials.size() - 1;
};

uint Actor::_addMesh(const aiMesh *mesh) {
  mMeshes.emplace_back(mesh);
  uint meshIdx = mMeshes.size() - 1;
  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  mMeshes[meshIdx].packVAO(VAO, VBO, EBO, mesh->mMaterialIndex);
  mMeshMap.push_back((MeshMap){
      .VAO = VAO, .VBO = VBO, .EBO = EBO, .materialId = mesh->mMaterialIndex});
  return meshIdx;
}
