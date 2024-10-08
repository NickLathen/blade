#include <filesystem>
#include <iostream>

#include "Shader.hpp"
#include "utils.hpp"

// expand #include directives
std::string LoadShaderSource(const std::string &path) {
  std::string src = LoadFileIntoString(path);
  const std::filesystem::path file_path(path);
  size_t replace_index;
  while ((replace_index = src.find("#include")) != src.npos) {
    size_t l_quote = src.find("\"", replace_index);
    size_t r_quote = src.find("\"", l_quote + 1);
    const std::string path = src.substr(l_quote + 1, r_quote - l_quote - 1);
    const std::string path_source =
        LoadShaderSource(file_path.parent_path() / path);
    src.replace(replace_index, r_quote - replace_index + 1, path_source);
  }
  return src;
}

Shader::Shader(const char *vertex_path, const char *frag_path) {
  const std::string vertex_source = LoadShaderSource(vertex_path);
  const std::string frag_source = LoadShaderSource(frag_path);
  const char *vert_c_str = vertex_source.c_str();
  const char *frag_c_str = frag_source.c_str();

  const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  const GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

  // compile shaders
  const uint kLogBufferSize = 512;
  int success;
  char info_log[kLogBufferSize]{};
  glShaderSource(vertex_shader, 1, &vert_c_str, nullptr);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, kLogBufferSize, NULL, info_log);
    throw std::runtime_error("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" +
                             std::string{info_log});
  }
  glShaderSource(frag_shader, 1, &frag_c_str, nullptr);
  glCompileShader(frag_shader);
  glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(frag_shader, kLogBufferSize, NULL, info_log);
    glDeleteShader(vertex_shader);
    throw std::runtime_error("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" +
                             std::string{info_log});
  }

  // link program
  m_program = glCreateProgram();
  glAttachShader(m_program, vertex_shader);
  glAttachShader(m_program, frag_shader);
  glLinkProgram(m_program);
  glGetProgramiv(m_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(m_program, kLogBufferSize, NULL, info_log);
    glDeleteShader(vertex_shader);
    glDeleteShader(frag_shader);
    throw std::runtime_error("ERROR::SHADER::PROGRAM::LINKING_FAILED\n" +
                             std::string{info_log});
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(frag_shader);
  std::cout << "Loaded shaders:" << vertex_path << ", " << frag_path
            << std::endl;
};
