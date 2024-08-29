#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <vector>

struct Camera {
  glm::mat4 transform{1.0f};
  glm::vec3 target{};
  float aspectRatio{};
  float fov{};
  float near{};
  float far{};
};

struct Light {
  glm::vec3 uAmbientLightColor;
  glm::vec3 uLightDir;
  glm::vec3 uLightPos;
  glm::vec3 uLightColor;
};

class RP_Material {
public:
  RP_Material();
  void init(const std::vector<Material> &materials,
            const std::vector<MeshVertexBuffer> &vertexBufferData,
            const std::vector<GLuint> &elementBufferData);
  void draw(const Camera &camera, const Light &light, const ::glm::mat4 &uMVP,
            const glm::mat4 &uLightMVP, GLuint FBO);
  GLuint getVBO() const;
  GLuint getEBO() const;

private:
  Shader mShader;
  GLuint mVAO, mVBO, mEBO, muMaterialUBO;
  GLuint mNumElements{0};
  const GLuint muMaterialBlockBinding{0};
};

class RP_ShadowMap {
public:
  RP_ShadowMap(GLuint VBO, GLuint EBO, GLuint numElements, GLuint textureSize);
  glm::mat4 getProjection();
  void draw(const glm::mat4 &uMVP);
  GLuint getFramebuffer();

private:
  Shader mDepthShader;
  GLuint mDepthVAO;
  GLuint mVBO;
  GLuint mEBO;
  GLuint mNumElements;
  GLuint mTextureSize;
  GLuint mDepthFBO;
  GLuint mDepthTexture;
};

class RP_Icon {
public:
  RP_Icon();
  void draw(const glm::vec4 &position, const glm::vec4 &color);

private:
  Shader mIconShader;
  GLuint mIconVAO;
};

class RP_Tex {
public:
  RP_Tex();
  void draw(GLuint texture);

private:
  Shader mTexShader;
  GLuint mTexVAO;
  GLuint mTexVBO;
};