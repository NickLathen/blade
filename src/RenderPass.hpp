#pragma once
#include "Shader.hpp"
#include "gl.hpp"

class RP_Depth {
public:
  RP_Depth(GLuint VBO, GLuint EBO, GLuint numElements, GLuint textureSize);
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