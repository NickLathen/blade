#pragma once

#include "gl.hpp"
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
  Shader(const char *vertex_path, const char *frag_path);
  ~Shader() {
    if (m_program != 0) {
      glDeleteProgram(m_program);
    }
  };
  NEVER_COPY(Shader);
  Shader(Shader &&other) : m_program{other.m_program} { other.m_program = 0; };
  inline void UseProgram() const;
  inline void UniformBlockBinding(const std::string &block_name,
                                  GLuint block_binding) const;
  inline void Uniform3fv(const std::string &name, const glm::vec3 &value) const;
  inline void Uniform4fv(const std::string &name, const glm::vec4 &value) const;
  inline void UniformMatrix4fv(const std::string &name, GLboolean transpose,
                               const glm::mat4 &value) const;
  inline void Uniform1f(const std::string &name, float value) const;
  inline void Uniform1i(const std::string &name, GLint value) const;
  inline void Uniform1ui(const std::string &name, GLuint value) const;

private:
  inline GLuint GetUniformLocation(const std::string &name) const;
  inline GLuint GetUniformBlockIndex(const std::string &block_name) const;
  GLuint m_program = 0;
};

inline void Shader::UseProgram() const { glUseProgram(m_program); };

inline GLuint Shader::GetUniformLocation(const std::string &name) const {
  return glGetUniformLocation(m_program, name.c_str());
};

inline GLuint
Shader::GetUniformBlockIndex(const std::string &block_name) const {
  return glGetUniformBlockIndex(m_program, block_name.c_str());
};

inline void Shader::UniformBlockBinding(const std::string &block_name,
                                        GLuint block_binding) const {
  GLuint uniform_block_index = GetUniformBlockIndex(block_name);
  return glUniformBlockBinding(m_program, uniform_block_index, block_binding);
};

inline void Shader::Uniform3fv(const std::string &name,
                               const glm::vec3 &value) const {
  glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
};
inline void Shader::Uniform4fv(const std::string &name,
                               const glm::vec4 &value) const {
  glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
};
inline void Shader::UniformMatrix4fv(const std::string &name,
                                     GLboolean transpose,
                                     const glm::mat4 &value) const {
  glUniformMatrix4fv(GetUniformLocation(name), 1, transpose,
                     glm::value_ptr(value));
};
inline void Shader::Uniform1f(const std::string &name, float value) const {
  glUniform1f(GetUniformLocation(name), value);
};

inline void Shader::Uniform1i(const std::string &name, GLint value) const {
  glUniform1i(GetUniformLocation(name), value);
};
inline void Shader::Uniform1ui(const std::string &name, GLuint value) const {
  glUniform1ui(GetUniformLocation(name), value);
};
