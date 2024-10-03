#pragma once

#include "ImageData.hpp"
#include "gl.hpp"
#include "utils.hpp"

class GpuVBO {
public:
  GpuVBO() { glGenBuffers(1, &m_vbo); }
  ~GpuVBO() {
    if (m_vbo != 0) {
      glDeleteBuffers(1, &m_vbo);
    };
  }
  NEVER_COPY(GpuVBO);
  GpuVBO(GpuVBO &&other) : m_vbo{other.m_vbo} { other.m_vbo = 0; };
  void BufferData(GLsizeiptr size, const void *data, GLenum usage) const {
    BindBuffer();
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    Unbind();
  };
  void BindBuffer() const { glBindBuffer(GL_ARRAY_BUFFER, m_vbo); }
  void Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

private:
  GLuint m_vbo;
};

class GpuVAO {
public:
  GpuVAO() { glGenVertexArrays(1, &m_vao); };
  ~GpuVAO() {
    if (m_vao != 0) {
      glDeleteVertexArrays(1, &m_vao);
    };
  }
  NEVER_COPY(GpuVAO);
  GpuVAO(GpuVAO &&other) : m_vao{other.m_vao} { other.m_vao = 0; };

  void BindVertexArray() const { glBindVertexArray(m_vao); };
  void VertexAttribPointer(const GpuVBO &vbo, GLuint index, GLint size,
                           GLenum type, GLboolean normalized, GLsizei stride,
                           const void *offset) const {
    glEnableVertexAttribArray(index);
    vbo.BindBuffer();
    glVertexAttribPointer(index, size, type, normalized, stride, offset);
    vbo.Unbind();
  };
  void VertexAttribIPointer(const GpuVBO &vbo, GLuint index, GLint size,
                            GLenum type, GLsizei stride,
                            const void *offset) const {
    glEnableVertexAttribArray(index);
    vbo.BindBuffer();
    glVertexAttribIPointer(index, size, type, stride, offset);
    vbo.Unbind();
  };
  void Unbind() const { glBindVertexArray(0); };

private:
  GLuint m_vao;
};

class GpuEBO {
public:
  GpuEBO() { glGenBuffers(1, &m_ebo); }
  ~GpuEBO() {
    if (m_ebo != 0) {
      glDeleteBuffers(1, &m_ebo);
    }
  }
  NEVER_COPY(GpuEBO);
  GpuEBO(GpuEBO &&other) : m_ebo{other.m_ebo} { other.m_ebo = 0; };

  void BufferData(GLsizeiptr size, const void *data, GLenum usage) const {
    BindBuffer();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
    Unbind();
  };
  void BindBuffer() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo); }
  void Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

private:
  GLuint m_ebo;
};

class GpuUBO {
public:
  GpuUBO() { glGenBuffers(1, &m_ubo); };
  ~GpuUBO() {
    if (m_ubo != 0) {
      glDeleteBuffers(1, &m_ubo);
    };
  }
  NEVER_COPY(GpuUBO);
  GpuUBO(GpuUBO &&other) : m_ubo{other.m_ubo} { other.m_ubo = 0; };

  void BindBufferBase(GLuint block_binding_index) const {
    glBindBufferBase(GL_UNIFORM_BUFFER, block_binding_index, m_ubo);
  };
  void BindBuffer() const { glBindBuffer(GL_UNIFORM_BUFFER, m_ubo); }
  void BufferData(GLsizeiptr size, const void *data, GLenum usage) const {
    BindBuffer();
    glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
    Unbind();
  };
  void BufferSubData(GLintptr offset, GLsizeiptr size, const void *data) const {
    BindBuffer();
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    Unbind();
  };
  void Unbind() const { glBindBuffer(GL_UNIFORM_BUFFER, 0); }

private:
  GLuint m_ubo;
};

class GpuFBO {
public:
  GpuFBO() { glGenFramebuffers(1, &m_fbo); }
  ~GpuFBO() {
    if (m_fbo != 0) {
      glDeleteFramebuffers(1, &m_fbo);
    }
  }
  NEVER_COPY(GpuFBO);
  GpuFBO(GpuFBO &&other) : m_fbo{other.m_fbo} { other.m_fbo = 0; };

  void BindFramebuffer(GLenum target) const {
    glBindFramebuffer(target, m_fbo);
  }
  void UnbindFramebuffer(GLenum target) const { glBindFramebuffer(target, 0); }
  GLenum CheckFramebufferStatus(GLenum target) const {
    return glCheckFramebufferStatus(target);
  }

private:
  GLuint m_fbo;
};