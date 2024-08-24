#pragma once

#include "gl.hpp"

class Shader {
public:
  Shader(const char *vertexPath, const char *fragPath);
  Shader(Shader const &other) = delete;
  Shader(Shader &&other) = delete;
  ~Shader();
  void useProgram() const;
  GLuint getAttribLocation(const std::string &attribName) const;
  GLuint getUniformLocation(const std::string &uniformName) const;

private:
  GLuint _program = 0;
};
