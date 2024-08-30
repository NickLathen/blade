#pragma once

#include "gl.hpp"
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
  Shader(const char *vertexPath, const char *fragPath);
  Shader(Shader const &other) = delete;
  Shader(Shader &&other) = delete;
  ~Shader();
  void useProgram() const;
  GLuint getAttribLocation(const std::string &attribName) const;
  GLuint getUniformLocation(const std::string &uniformName) const;
  GLuint getUniformBlockIndex(const std::string &blockName) const;
  void setUniformBlockBinding(const std::string &blockName,
                              GLuint uniformBlockBinding) const;
  void uniform3fv(const std::string &uniformName, const glm::vec3 &value) const;
  void uniform4fv(const std::string &uniformName, const glm::vec4 &value) const;
  void uniformMatrix4fv(const std::string &uniformName, GLboolean transpose,
                        const glm::mat4 &value) const;
  void uniform1f(const std::string &uniformName, float value) const;
  void uniform1i(const std::string &uniformName, GLint value) const;
  void uniform1ui(const std::string &uniformName, GLuint value) const;

private:
  GLuint _program = 0;
};
