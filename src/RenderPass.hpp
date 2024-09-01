#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include "utils.hpp"
#include <utility>
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

class RP_VBO {
public:
  RP_VBO() { glGenBuffers(1, &mVBO); }
  ~RP_VBO() {
    if (mVBO != 0) {
      glDeleteBuffers(1, &mVBO);
    };
  }
  NEVER_COPY(RP_VBO);
  RP_VBO(RP_VBO &&other) : mVBO{other.mVBO} { other.mVBO = 0; };
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
  ~RP_VAO() {
    if (mVAO != 0) {
      glDeleteVertexArrays(1, &mVAO);
    };
  }
  NEVER_COPY(RP_VAO);
  RP_VAO(RP_VAO &&other) : mVAO{other.mVAO} { other.mVAO = 0; };

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
  ~RP_EBO() {
    if (mEBO != 0) {
      glDeleteBuffers(1, &mEBO);
    }
  }
  NEVER_COPY(RP_EBO);
  RP_EBO(RP_EBO &&other) : mEBO{other.mEBO} { other.mEBO = 0; };

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
  ~RP_UBO() {
    if (mUBO != 0) {
      glDeleteBuffers(1, &mUBO);
    };
  }
  NEVER_COPY(RP_UBO);
  RP_UBO(RP_UBO &&other) : mUBO{other.mUBO} { other.mUBO = 0; };

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
  ~RP_FBO() {
    if (mFBO != 0) {
      glDeleteFramebuffers(1, &mFBO);
    }
  }
  NEVER_COPY(RP_FBO);
  RP_FBO(RP_FBO &&other) : mFBO{other.mFBO} { other.mFBO = 0; };

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
  ~RP_Texture() {
    if (mTexture != 0) {
      glDeleteTextures(1, &mTexture);
    }
  }
  NEVER_COPY(RP_Texture);
  RP_Texture(RP_Texture &&other) : mTexture{other.mTexture} {
    other.mTexture = 0;
  };

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
  NEVER_COPY(RP_Material);
  RP_Material(RP_Material &&other)
      : mShader{std::move(other.mShader)}, mVAO{std::move(other.mVAO)},
        mUBO{std::move(other.mUBO)}, mVBO{std::move(other.mVBO)},
        mEBO{std::move(other.mEBO)}, mNumElements{other.mNumElements},
        muMaterialBlockBinding{other.muMaterialBlockBinding},
        muLightDepthTexture{other.muLightDepthTexture} {};

  void draw(const glm::vec3 &uCameraPos, const Light &light,
            const ::glm::mat4 &uMVP, const glm::mat4 &uLightMVP,
            const glm::mat4 &uModelMatrix, const RP_FBO &FBO) const;
  void drawVertices() const;
  const RP_VBO &getVBO() const;
  const RP_EBO &getEBO() const;

private:
  Shader mShader;
  RP_VAO mVAO;
  RP_UBO mUBO;
  RP_VBO mVBO;
  RP_EBO mEBO;
  GLuint mNumElements{0};
  const GLuint muMaterialBlockBinding{0};
  const GLuint muLightDepthTexture{0};
};

class RP_DepthMap {
public:
  RP_DepthMap(GLuint textureSize);
  NEVER_COPY(RP_DepthMap);
  RP_DepthMap(RP_DepthMap &&other)
      : mDepthShader{std::move(other.mDepthShader)},
        mFBO{std::move(other.mFBO)}, mTexture{std::move(other.mTexture)},
        mTextureSize{other.mTextureSize}, gVP{other.gVP},
        gDepthTest{other.gDepthTest}, gCullFace{other.gCullFace} {};
  glm::mat4 getProjection() const;
  void begin();
  void setMVP(const glm::mat4 &uMVP);
  void end();
  const RP_FBO &getFBO() const;

private:
  Shader mDepthShader;
  RP_FBO mFBO;
  RP_Texture mTexture;
  GLuint mTextureSize{0};
  glm::ivec4 gVP{};
  GLboolean gDepthTest;
  GLboolean gCullFace;
};

class RP_Terrain {
public:
  RP_Terrain();
  NEVER_COPY(RP_Terrain);
  RP_Terrain(RP_Terrain &&other)
      : mShader{std::move(other.mShader)}, mVAO{std::move(other.mVAO)},
        mVBO{std::move(other.mVBO)}, mEBO{std::move(other.mEBO)},
        mUBO{std::move(other.mUBO)}, mTexture{std::move(other.mTexture)},
        muLightDepthTexture{other.muLightDepthTexture},
        muDiffuseTexture{other.muDiffuseTexture},
        muMaterialBlockBinding{other.muMaterialBlockBinding},
        mNumElements{other.mNumElements} {};
  void draw(const glm::vec3 &uCameraPos, const Light &light,
            const glm::mat4 &uTerrainMVP, const glm::mat4 &uLightMVP,
            const glm::mat4 &uTerrainMatrix, const RP_FBO &FBO) const;
  void drawVertices() const;

private:
  Shader mShader;
  RP_VAO mVAO;
  RP_VBO mVBO;
  RP_EBO mEBO;
  RP_UBO mUBO;
  RP_Texture mTexture;
  const GLuint muLightDepthTexture{0};
  const GLuint muDiffuseTexture{1};
  const GLuint muMaterialBlockBinding{0};
  GLuint mNumElements{0};
};

class RP_Icon {
public:
  RP_Icon();
  NEVER_COPY(RP_Icon);
  RP_Icon(RP_Icon &&other)
      : mIconShader{std::move(other.mIconShader)},
        mVAO{std::move(other.mVAO)} {};
  void draw(const glm::vec4 &position, const glm::vec4 &color) const;

private:
  Shader mIconShader;
  RP_VAO mVAO;
};

class RP_Tex {
public:
  RP_Tex();
  NEVER_COPY(RP_Tex);
  RP_Tex(RP_Tex &&other)
      : mTexShader{std::move(other.mTexShader)}, mVAO{std::move(other.mVAO)},
        mVBO{std::move(other.mVBO)},
        muTextureBinding{other.muTextureBinding} {};
  void draw(const RP_FBO &FBO) const;

private:
  Shader mTexShader;
  RP_VAO mVAO;
  RP_VBO mVBO;
  const GLuint muTextureBinding{1};
};