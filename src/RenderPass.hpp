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
  float aspect_ratio{};
  float fov{};
  float near{};
  float far{};
};

struct Light {
  glm::vec3 ambient_color;
  glm::vec3 direction;
  glm::vec3 position;
  glm::vec3 diffuse_color;
};

struct TextureTileConfig {
  float repeat_scale;
  float rotation_scale;
  float translation_scale;
  float noise_scale;
  float hue_scale;
  float saturation_scale;
  float brightness_scale;
};

class VBO {
public:
  VBO() { glGenBuffers(1, &m_vbo); }
  ~VBO() {
    if (m_vbo != 0) {
      glDeleteBuffers(1, &m_vbo);
    };
  }
  NEVER_COPY(VBO);
  VBO(VBO &&other) : m_vbo{other.m_vbo} { other.m_vbo = 0; };
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

class VAO {
public:
  VAO() { glGenVertexArrays(1, &m_vao); };
  ~VAO() {
    if (m_vao != 0) {
      glDeleteVertexArrays(1, &m_vao);
    };
  }
  NEVER_COPY(VAO);
  VAO(VAO &&other) : m_vao{other.m_vao} { other.m_vao = 0; };

  void BindVertexArray() const { glBindVertexArray(m_vao); };
  void VertexAttribPointer(const VBO &vbo, GLuint index, GLint size,
                           GLenum type, GLboolean normalized, GLsizei stride,
                           const void *offset) const {
    glEnableVertexAttribArray(index);
    vbo.BindBuffer();
    glVertexAttribPointer(index, size, type, normalized, stride, offset);
    vbo.Unbind();
  };
  void VertexAttribIPointer(const VBO &vbo, GLuint index, GLint size,
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

class EBO {
public:
  EBO() { glGenBuffers(1, &m_ebo); }
  ~EBO() {
    if (m_ebo != 0) {
      glDeleteBuffers(1, &m_ebo);
    }
  }
  NEVER_COPY(EBO);
  EBO(EBO &&other) : m_ebo{other.m_ebo} { other.m_ebo = 0; };

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

class UBO {
public:
  UBO() { glGenBuffers(1, &m_ubo); };
  ~UBO() {
    if (m_ubo != 0) {
      glDeleteBuffers(1, &m_ubo);
    };
  }
  NEVER_COPY(UBO);
  UBO(UBO &&other) : m_ubo{other.m_ubo} { other.m_ubo = 0; };

  void BindBufferBase(GLuint block_binding_index) const {
    glBindBufferBase(GL_UNIFORM_BUFFER, block_binding_index, m_ubo);
  };
  void BindBuffer() const { glBindBuffer(GL_UNIFORM_BUFFER, m_ubo); }
  void Unbind() const { glBindBuffer(GL_UNIFORM_BUFFER, 0); }
  void BufferData(GLsizeiptr size, const void *data, GLenum usage) const {
    BindBuffer();
    glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
    Unbind();
  };

private:
  GLuint m_ubo;
};

class FBO {
public:
  FBO() { glGenFramebuffers(1, &m_fbo); }
  ~FBO() {
    if (m_fbo != 0) {
      glDeleteFramebuffers(1, &m_fbo);
    }
  }
  NEVER_COPY(FBO);
  FBO(FBO &&other) : m_fbo{other.m_fbo} { other.m_fbo = 0; };

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

class RPTexture {
public:
  RPTexture() { glGenTextures(1, &m_texture); }
  ~RPTexture() {
    if (m_texture != 0) {
      glDeleteTextures(1, &m_texture);
    }
  }
  NEVER_COPY(RPTexture);
  RPTexture(RPTexture &&other) : m_texture{other.m_texture} {
    other.m_texture = 0;
  };

  void BindTexture(GLenum target) const { glBindTexture(target, m_texture); }
  void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                            GLint level) const {
    glFramebufferTexture2D(target, attachment, textarget, m_texture, level);
  }

private:
  GLuint m_texture;
};

class RPMaterialShader {
public:
  RPMaterialShader()
      : m_shader{"shaders/vertex.glsl", "shaders/fragment.glsl"} {
    m_shader.UseProgram();
    m_shader.UniformBlockBinding("uMaterialBlock", m_material_block_binding);
    m_shader.Uniform1i("uDepthTexture", m_depth_texture);
    m_shader.Uniform1i("uDiffuseTexture", m_diffuse_texture);
    m_shader.Uniform1i("uBlendTexture", m_blend_texture);
    m_shader.Uniform1i("uNoiseTexture", m_noise_texture);
    glUseProgram(0);
  };
  NEVER_COPY(RPMaterialShader);
  RPMaterialShader(RPMaterialShader &&other)
      : m_shader{std::move(other.m_shader)},
        m_depth_texture{other.m_depth_texture},
        m_diffuse_texture{other.m_diffuse_texture},
        m_blend_texture{other.m_blend_texture},
        m_noise_texture{other.m_noise_texture},
        m_material_block_binding{other.m_material_block_binding} {};
  void Begin() {
    glGetBooleanv(GL_DEPTH_TEST, &g_depth_test);
    glGetBooleanv(GL_CULL_FACE, &g_cull_face);
    glGetIntegerv(GL_CULL_FACE_MODE, &g_cull_face_mode);
    glGetIntegerv(GL_FRONT_FACE, &g_front_face);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
  }
  void SetUniforms(const glm::vec3 &camera_pos, const Light &light,
                   const TextureTileConfig &tileConfig, const glm::mat4 &mvp,
                   const glm::mat4 &light_mvp,
                   const glm::mat4 &model_matrix) const {
    m_shader.UseProgram();
    m_shader.Uniform3fv("uCameraPos", camera_pos);
    m_shader.Uniform3fv("uAmbientLightColor", light.ambient_color);
    m_shader.Uniform3fv("uLightDir", light.direction);
    m_shader.Uniform3fv("uLightColor", light.diffuse_color);
    m_shader.Uniform3fv("uLightPos", light.position);
    m_shader.UniformMatrix4fv("uMVP", GL_FALSE, mvp);
    m_shader.UniformMatrix4fv("uLightMVP", GL_FALSE, light_mvp);
    m_shader.UniformMatrix4fv("uModelMatrix", GL_FALSE, model_matrix);
    m_shader.Uniform1f("uSpecularPower", 32.0f);
    m_shader.Uniform1f("uShininessScale", 2000.0f);

    m_shader.Uniform1f("uRepeatScale", tileConfig.repeat_scale);
    m_shader.Uniform1f("uRotationScale", tileConfig.rotation_scale);
    m_shader.Uniform1f("uTranslationScale", tileConfig.translation_scale);
    m_shader.Uniform1f("uNoiseScale", tileConfig.noise_scale);
    m_shader.Uniform1f("uHueOffset", tileConfig.hue_scale);
    m_shader.Uniform1f("uSaturation", tileConfig.saturation_scale);
    m_shader.Uniform1f("uBrightness", tileConfig.brightness_scale);
  }
  void BindMaterialsBuffer(const UBO &ubo) const {
    ubo.BindBufferBase(m_material_block_binding);
  }
  void BindTexture(const RPTexture &texture,
                   const GLuint texture_location) const {
    glActiveTexture(GL_TEXTURE0 + texture_location);
    texture.BindTexture(GL_TEXTURE_2D);
  }
  void BindDepthTexture(const RPTexture &texture) const {
    BindTexture(texture, m_depth_texture);
  }
  void BindDiffuseTexture(const RPTexture &texture) const {
    BindTexture(texture, m_diffuse_texture);
  }
  void BindBlendTexture(const RPTexture &texture) const {
    BindTexture(texture, m_blend_texture);
  }
  void BindNoiseTexture(const RPTexture &texture) const {
    BindTexture(texture, m_noise_texture);
  }
  void End() {
    if (g_depth_test == GL_FALSE)
      glDisable(GL_DEPTH_TEST);
    if (g_cull_face == GL_FALSE)
      glDisable(GL_CULL_FACE);
    glCullFace(g_cull_face_mode);
    glFrontFace(g_front_face);
  }

private:
  Shader m_shader;
  const GLuint m_depth_texture{0};
  const GLuint m_diffuse_texture{1};
  const GLuint m_blend_texture{2};
  const GLuint m_noise_texture{3};
  const GLuint m_material_block_binding{0};
  GLboolean g_depth_test, g_cull_face;
  GLint g_cull_face_mode, g_front_face;
};

class RPMaterial {
public:
  RPMaterial(const std::vector<Material> &materials,
             const std::vector<MeshVertexBuffer> &vertex_buffer_data,
             const std::vector<GLuint> &element_buffer_data);
  NEVER_COPY(RPMaterial);
  RPMaterial(RPMaterial &&other)
      : m_vao{std::move(other.m_vao)}, m_ubo{std::move(other.m_ubo)},
        m_vbo{std::move(other.m_vbo)}, m_ebo{std::move(other.m_ebo)},
        m_num_elements{other.m_num_elements} {};

  void DrawVertices() const;
  const UBO &GetMaterialsBuffer() const { return m_ubo; };

private:
  VAO m_vao;
  UBO m_ubo;
  VBO m_vbo;
  EBO m_ebo;
  GLuint m_num_elements{0};
};

class RPDepthMap {
public:
  RPDepthMap(GLuint texture_size);
  NEVER_COPY(RPDepthMap);
  RPDepthMap(RPDepthMap &&other)
      : m_shader{std::move(other.m_shader)}, m_fbo{std::move(other.m_fbo)},
        m_texture{std::move(other.m_texture)},
        m_texture_size{other.m_texture_size}, g_vp{other.g_vp},
        g_depth_test{other.g_depth_test}, g_cull_face{other.g_cull_face} {};
  glm::mat4 GetProjection() const;
  void Begin();
  void SetMVP(const glm::mat4 &mvp);
  void End();
  const RPTexture &GetTexture() const;

private:
  Shader m_shader;
  FBO m_fbo;
  RPTexture m_texture;
  GLuint m_texture_size{0};
  glm::ivec4 g_vp{};
  GLboolean g_depth_test;
  GLboolean g_cull_face;
};

class RPTerrain {
public:
  RPTerrain();
  NEVER_COPY(RPTerrain);
  RPTerrain(RPTerrain &&other)
      : m_vao{std::move(other.m_vao)}, m_vbo{std::move(other.m_vbo)},
        m_ebo{std::move(other.m_ebo)}, m_ubo{std::move(other.m_ubo)},
        m_num_elements{other.m_num_elements} {};
  void DrawVertices() const;
  const UBO &GetMaterialsBuffer() const { return m_ubo; };

private:
  VAO m_vao;
  VBO m_vbo;
  EBO m_ebo;
  UBO m_ubo;
  GLuint m_num_elements{0};
};

class RPIcon {
public:
  RPIcon();
  NEVER_COPY(RPIcon);
  RPIcon(RPIcon &&other)
      : m_shader{std::move(other.m_shader)}, m_vao{std::move(other.m_vao)} {};
  void Draw(const glm::vec4 &position, const glm::vec4 &color) const;

private:
  Shader m_shader;
  VAO m_vao;
};

class RPTex {
public:
  RPTex();
  NEVER_COPY(RPTex);
  RPTex(RPTex &&other)
      : m_shader{std::move(other.m_shader)}, m_vao{std::move(other.m_vao)},
        m_vbo{std::move(other.m_vbo)},
        m_texture_binding{other.m_texture_binding} {};
  void Draw(const RPTexture &texture) const;

private:
  Shader m_shader;
  VAO m_vao;
  VBO m_vbo;
  const GLuint m_texture_binding{1};
};