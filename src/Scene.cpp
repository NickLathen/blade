#include "Scene.hpp"

Scene::Scene(const aiScene *scene) {
  auto processNode = [&](auto &processNode, const aiNode *node) -> void {
    for (uint i = 0; i < node->mNumMeshes; i++) {
      const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      const glm::mat4 iMatrix{1.0};
      _addEntity(mesh, mesh->mMaterialIndex, iMatrix);
    }
    for (uint i = 0; i < node->mNumChildren; i++) {
      processNode(processNode, node->mChildren[i]);
    }
  };
  for (uint i = 0; i < scene->mNumMaterials; i++) {
    _addMaterial(scene->mMaterials[i]);
  }
  processNode(processNode, scene->mRootNode);
};

void Scene::draw(const Shader &shader, const glm::mat4 &viewProjection) {
  for (uint i = 0; i < mEntities.size(); i++) {
    const Entity &entity = mEntities[i];
    const uint materialIdx = mMap_entity_material[i];
    const Material &material = mMaterials[materialIdx];
    const uint meshIdx = mMap_entity_mesh[i];
    const Mesh &mesh = mMeshes[meshIdx];
    const glm::mat4 model = entity.getTransform();
    glm::mat4 mvp = viewProjection * model;
    glUniformMatrix4fv(shader.getUniformLocation("uMVP"), 1, GL_FALSE,
                       glm::value_ptr(mvp));
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix3fv(shader.getUniformLocation("uNormalMatrix"), 1, GL_FALSE,
                       glm::value_ptr(normalMatrix));
    glUniform3fv(shader.getUniformLocation("uColor"), 1,
                 glm::value_ptr(material.getDiffuse()));
    glBindVertexArray(mMap_mesh_vao[meshIdx]);
    glDrawElements(GL_TRIANGLES, mesh.getNumElements(), GL_UNSIGNED_INT, 0);
  }
  glBindVertexArray(0);
}

uint Scene::_addEntity(const aiMesh *mesh, uint materialIdx,
                       const glm::mat4 &transform) {
  uint meshIdx = _addMesh(mesh);
  mMap_entity_mesh.push_back(meshIdx);
  mMap_entity_material.push_back(materialIdx);
  mEntities.emplace_back(transform);
  return mEntities.size() - 1;
}

uint Scene::_addMaterial(const aiMaterial *material) {
  mMaterials.emplace_back(material);
  return mMaterials.size() - 1;
};

uint Scene::_addMesh(const aiMesh *mesh) {
  mMeshes.emplace_back(mesh);
  uint idx = mMeshes.size() - 1;
  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  mMeshes[idx].packVAO(VAO, VBO, EBO);
  mMap_mesh_vao.push_back(VAO);
  mMap_mesh_vbo.push_back(VBO);
  mMap_mesh_ebo.push_back(EBO);
  return idx;
}
