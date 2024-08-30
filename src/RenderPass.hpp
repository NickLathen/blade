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

#define RP_NEVER_COPY(T)                                                       \
  T(const T &) = delete;                                                       \
  T &operator=(const T &) = delete;                                            \
  T(T &&) = delete;                                                            \
  T &operator=(T &&) = delete

class RP_VBO {
public:
  RP_VBO() { glGenBuffers(1, &mVBO); }
  ~RP_VBO() { glDeleteBuffers(1, &mVBO); }
  RP_NEVER_COPY(RP_VBO);
  void bufferData(GLsizeiptr size, const void *data, GLenum usage) const {
    bindBuffer();
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    unbind();
  };
  void bindBuffer() const { glBindBuffer(GL_ARRAY_BUFFER, mVBO); }
  void unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

private:
  GLuint mVBO;
};

class RP_VAO {
public:
  RP_VAO() { glGenVertexArrays(1, &mVAO); };
  ~RP_VAO() { glDeleteVertexArrays(1, &mVAO); };
  RP_NEVER_COPY(RP_VAO);
  void bindVertexArray() const { glBindVertexArray(mVAO); };
  void vertexAttribPointer(const RP_VBO &VBO, GLuint index, GLint size,
                           GLenum type, GLboolean normalized, GLsizei stride,
                           const void *offset) const {
    glEnableVertexAttribArray(index);
    VBO.bindBuffer();
    glVertexAttribPointer(index, size, type, normalized, stride, offset);
    VBO.unbind();
  };
  void vertexAttribIPointer(const RP_VBO &VBO, GLuint index, GLint size,
                            GLenum type, GLsizei stride,
                            const void *offset) const {
    glEnableVertexAttribArray(index);
    VBO.bindBuffer();
    glVertexAttribIPointer(index, size, type, stride, offset);
    VBO.unbind();
  };
  void unbind() const { glBindVertexArray(0); };

private:
  GLuint mVAO;
};

class RP_EBO {
public:
  RP_EBO() { glGenBuffers(1, &mEBO); }
  ~RP_EBO() { glDeleteBuffers(1, &mEBO); }
  RP_NEVER_COPY(RP_EBO);
  void bufferData(GLsizeiptr size, const void *data, GLenum usage) const {
    bindBuffer();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
    unbind();
  };
  void bindBuffer() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO); }
  void unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

private:
  GLuint mEBO;
};

class RP_UBO {
public:
  RP_UBO() { glGenBuffers(1, &mUBO); };
  ~RP_UBO() { glDeleteBuffers(1, &mUBO); };
  RP_NEVER_COPY(RP_UBO);
  void bindBufferBase(GLuint blockBindingIndex) const {
    glBindBufferBase(GL_UNIFORM_BUFFER, blockBindingIndex, mUBO);
  };
  void bindBuffer() const { glBindBuffer(GL_UNIFORM_BUFFER, mUBO); }
  void unbind() const { glBindBuffer(GL_UNIFORM_BUFFER, 0); }
  void bufferData(GLsizeiptr size, const void *data, GLenum usage) const {
    bindBuffer();
    glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
    unbind();
  };

private:
  GLuint mUBO;
};

class RP_FBO {
public:
  RP_FBO() { glGenFramebuffers(1, &mFBO); }
  ~RP_FBO() { glDeleteFramebuffers(1, &mFBO); }
  RP_NEVER_COPY(RP_FBO);
  void bindTexture(GLenum target) const { glBindTexture(target, mFBO); }
  void bindFramebuffer(GLenum target) const { glBindFramebuffer(target, mFBO); }
  void unbindFramebuffer(GLenum target) const { glBindFramebuffer(target, 0); }
  GLenum checkFramebufferStatus(GLenum target) const {
    return glCheckFramebufferStatus(target);
  }

private:
  GLuint mFBO;
};

class RP_Texture {
public:
  RP_Texture() { glGenTextures(1, &mTexture); }
  ~RP_Texture() { glDeleteTextures(1, &mTexture); }
  RP_NEVER_COPY(RP_Texture);
  void bindTexture(GLenum target) const { glBindTexture(target, mTexture); }
  void framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                            GLint level) const {
    glFramebufferTexture2D(target, attachment, textarget, mTexture, level);
  }

private:
  GLuint mTexture;
};

class RP_Material {
public:
  RP_Material(const std::vector<Material> &materials,
              const std::vector<MeshVertexBuffer> &vertexBufferData,
              const std::vector<GLuint> &elementBufferData);
  RP_NEVER_COPY(RP_Material);
  void draw(const glm::vec3 &uCameraPos, const Light &light,
            const ::glm::mat4 &uMVP, const glm::mat4 &uLightMVP,
            const glm::mat4 &uModelMatrix, const RP_FBO &FBO) const;
  const RP_VBO &getVBO() const;
  const RP_EBO &getEBO() const;

private:
  Shader mShader;
  const RP_VAO mVAO;
  const RP_UBO mUBO;
  const RP_VBO mVBO;
  const RP_EBO mEBO;
  GLuint mNumElements{0};
  const GLuint muMaterialBlockBinding{0};
  const GLuint muTextureBinding{0};
};

class RP_ShadowMap {
public:
  RP_ShadowMap(const RP_VBO &VBO, const RP_EBO &EBO, GLuint numElements,
               GLuint textureSize);
  RP_NEVER_COPY(RP_ShadowMap);
  glm::mat4 getProjection() const;
  void draw(const glm::mat4 &uMVP) const;
  const RP_FBO &getFBO() const;

private:
  Shader mDepthShader;
  const RP_VAO mVAO;
  const RP_FBO mFBO;
  const RP_Texture mTexture;
  GLuint mNumElements{0};
  GLuint mTextureSize{0};
};

class RP_Icon {
public:
  RP_Icon();
  RP_NEVER_COPY(RP_Icon);
  void draw(const glm::vec4 &position, const glm::vec4 &color) const;

private:
  Shader mIconShader;
  const RP_VAO mVAO;
};

class RP_Tex {
public:
  RP_Tex();
  RP_NEVER_COPY(RP_Tex);
  void draw(const RP_FBO &FBO) const;

private:
  Shader mTexShader;
  const RP_VAO mVAO;
  const RP_VBO mVBO;
  const GLuint muTextureBinding{1};
};