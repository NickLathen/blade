#pragma once

#include "gl.hpp"
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
  Shader(const char *vertexPath, const char *fragPath);
  ~Shader() {
    if (_program != 0) {
      glDeleteProgram(_program);
    }
  };
  NEVER_COPY(Shader);
  Shader(Shader &&other) : _program{other._program} { other._program = 0; };
  inline void useProgram() const;
  inline void uniformBlockBlinding(const std::string &blockName,
                                   GLuint blockBinding) const;
  inline void uniform3fv(const std::string &uniformName,
                         const glm::vec3 &value) const;
  inline void uniform4fv(const std::string &uniformName,
                         const glm::vec4 &value) const;
  inline void uniformMatrix4fv(const std::string &uniformName,
                               GLboolean transpose,
                               const glm::mat4 &value) const;
  inline void uniform1f(const std::string &uniformName, float value) const;
  inline void uniform1i(const std::string &uniformName, GLint value) const;
  inline void uniform1ui(const std::string &uniformName, GLuint value) const;

private:
  inline GLuint getUniformLocation(const std::string &uniformName) const;
  inline GLuint getUniformBlockIndex(const std::string &blockName) const;
  GLuint _program = 0;
};

inline void Shader::useProgram() const { glUseProgram(_program); };

inline GLuint Shader::getUniformLocation(const std::string &uniformName) const {
  return glGetUniformLocation(_program, uniformName.c_str());
};

inline GLuint Shader::getUniformBlockIndex(const std::string &blockName) const {
  return glGetUniformBlockIndex(_program, blockName.c_str());
};

inline void Shader::uniformBlockBlinding(const std::string &blockName,
                                         GLuint blockBinding) const {
  GLuint uniformBlockIndex = getUniformBlockIndex(blockName);
  return glUniformBlockBinding(_program, uniformBlockIndex, blockBinding);
};

inline void Shader::uniform3fv(const std::string &uniformName,
                               const glm::vec3 &value) const {
  glUniform3fv(getUniformLocation(uniformName), 1, glm::value_ptr(value));
};
inline void Shader::uniform4fv(const std::string &uniformName,
                               const glm::vec4 &value) const {
  glUniform4fv(getUniformLocation(uniformName), 1, glm::value_ptr(value));
};
inline void Shader::uniformMatrix4fv(const std::string &uniformName,
                                     GLboolean transpose,
                                     const glm::mat4 &value) const {
  glUniformMatrix4fv(getUniformLocation(uniformName), 1, transpose,
                     glm::value_ptr(value));
};
inline void Shader::uniform1f(const std::string &uniformName,
                              float value) const {
  glUniform1f(getUniformLocation(uniformName), value);
};

inline void Shader::uniform1i(const std::string &uniformName,
                              GLint value) const {
  glUniform1i(getUniformLocation(uniformName), value);
};
inline void Shader::uniform1ui(const std::string &uniformName,
                               GLuint value) const {
  glUniform1ui(getUniformLocation(uniformName), value);
};
