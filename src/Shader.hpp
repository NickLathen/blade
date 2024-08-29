#pragma once

#include "gl.hpp"
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

private:
  GLuint _program = 0;
};
