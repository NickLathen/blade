#pragma once
#include <SDL.h>
#include <utility>
#include <vector>

#include "GPUResources.hpp"
#include "GpuTexture.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "MeshGroup.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include "utils.hpp"

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
  glm::vec3 diffuse_color;
  float static_distance;
  float static_fov;
  float static_bias;
};

struct TextureTileConfig {
  float height_scale;
  float width_scale;
  float grid_scale;
  int resolution;
  float repeat_scale;
  float rotation_scale;
  float translation_scale;
  float noise_scale;
  float hue_scale;
  float saturation_scale;
  float brightness_scale;
  float flat_bias;
  float parallel_bias;
};

class RPMaterialShader {
public:
  RPMaterialShader()
      : m_shader{"shaders/vertex.glsl", "shaders/fragment.glsl"},
        m_depth_shader{"shaders/vertex.glsl",
                       "shaders/depth_map_fragment.glsl"} {
    m_shader.UseProgram();
    m_shader.UniformBlockBinding("uMaterialBlock", m_material_block_binding);
    m_shader.Uniform1i("uDepthTexture", m_depth_texture);

    glUseProgram(0);
  };
  NEVER_COPY(RPMaterialShader);
  RPMaterialShader(RPMaterialShader &&other)
      : m_shader{std::move(other.m_shader)},
        m_depth_shader{std::move(other.m_depth_shader)},
        m_depth_texture{other.m_depth_texture},
        m_material_block_binding{other.m_material_block_binding} {};
  void Begin() {
    m_shader.UseProgram();
    glGetBooleanv(GL_DEPTH_TEST, &g_depth_test);
    glGetBooleanv(GL_CULL_FACE, &g_cull_face);
    glGetIntegerv(GL_CULL_FACE_MODE, &g_cull_face_mode);
    glGetIntegerv(GL_FRONT_FACE, &g_front_face);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
  }
  void BeginDepth() { m_depth_shader.UseProgram(); }
  void EndDepth() { glUseProgram(0); }
  void SetUniforms(const glm::vec3 &camera_pos, const Light &light,
                   const glm::mat4 &mvp, const glm::mat4 &light_mvp,
                   const glm::mat4 &model_matrix) const {
    m_shader.Uniform3fv("uCameraPos", camera_pos);
    m_shader.Uniform3fv("uAmbientLightColor", light.ambient_color);
    m_shader.Uniform3fv("uLightDir", light.direction);
    m_shader.Uniform3fv("uLightColor", light.diffuse_color);
    m_shader.UniformMatrix4fv("uMVP", GL_FALSE, mvp);
    m_shader.UniformMatrix4fv("uLightMVP", GL_FALSE, light_mvp);
    m_shader.UniformMatrix4fv("uModelMatrix", GL_FALSE, model_matrix);
    m_shader.Uniform1f("uSpecularPower", 32.0f);
    m_shader.Uniform1f("uShininessScale", 2000.0f);
  }
  void SetDepthUniforms(const glm::mat4 &mvp, const glm::mat4 &light_mvp,
                        const glm::mat4 &model_matrix) const {
    m_depth_shader.UniformMatrix4fv("uMVP", GL_FALSE, mvp);
    m_depth_shader.UniformMatrix4fv("uLightMVP", GL_FALSE, light_mvp);
    m_depth_shader.UniformMatrix4fv("uModelMatrix", GL_FALSE, model_matrix);
  }
  void BindMaterialsBuffer(const GpuUBO &ubo) const {
    ubo.BindBufferBase(m_material_block_binding);
  }
  void BindTexture(const GpuTexture &texture,
                   const GLuint texture_location) const {
    glActiveTexture(GL_TEXTURE0 + texture_location);
    texture.BindTexture(GL_TEXTURE_2D);
  }
  void BindDepthTexture(const GpuTexture &texture) const {
    BindTexture(texture, m_depth_texture);
  }
  void BindTextures(std::vector<GpuTexture>::const_iterator start,
                    std::vector<GpuTexture>::const_iterator end) const {
    std::vector<GLint> samplers{};
    int offset = 0;
    for (int i = 0; start != end; i++, start++) {
      if (i == m_depth_texture)
        offset++;
      BindTexture(*start, i + offset);
      samplers.push_back(i + offset);
    }
    m_shader.Uniform1iv("uTextures", samplers.size(), &samplers[0]);
  }
  void BindTextures(const std::vector<GpuTexture> &textures) const {
    BindTextures(textures.begin(), textures.end());
  }
  void End() {
    if (g_depth_test == GL_FALSE)
      glDisable(GL_DEPTH_TEST);
    if (g_cull_face == GL_FALSE)
      glDisable(GL_CULL_FACE);
    glCullFace(g_cull_face_mode);
    glFrontFace(g_front_face);
    glUseProgram(0);
  }

private:
  Shader m_shader;
  Shader m_depth_shader;
  const GLuint m_depth_texture{1};
  const GLuint m_material_block_binding{0};
  GLboolean g_depth_test, g_cull_face;
  GLint g_cull_face_mode, g_front_face;
};

class RPTerrainShader {
public:
  RPTerrainShader()
      : m_shader{"shaders/terrain_vertex.glsl",
                 "shaders/terrain_fragment.glsl"},
        m_depth_shader{"shaders/terrain_vertex.glsl",
                       "shaders/depth_map_fragment.glsl"},
        m_depth_skirt_shader{"shaders/terrain_skirt_vertex.glsl",
                             "shaders/depth_map_fragment.glsl"} {

    m_tile_config_ubo.BufferData(sizeof(TextureTileConfig), NULL,
                                 GL_DYNAMIC_DRAW);

    m_shader.UseProgram();
    m_shader.UniformBlockBinding("uMaterialBlock", m_material_block_binding);
    m_shader.UniformBlockBinding("uTileConfigBlock",
                                 m_tile_config_block_binding);
    m_shader.Uniform1i("uDepthTexture", m_depth_texture);
    m_shader.Uniform1i("uNoiseTexture", m_noise_texture);
    m_shader.Uniform1i("uHeightmapTexture", m_heightmap_texture);
    m_shader.Uniform1i("uBlendTexture", m_blend_texture);

    m_depth_shader.UseProgram();
    m_depth_shader.UniformBlockBinding("uTileConfigBlock",
                                       m_tile_config_block_binding);
    m_depth_shader.Uniform1i("uHeightmapTexture", m_heightmap_texture);

    m_depth_skirt_shader.UseProgram();
    m_depth_skirt_shader.UniformBlockBinding("uTileConfigBlock",
                                             m_tile_config_block_binding);
    m_depth_skirt_shader.Uniform1i("uHeightmapTexture", m_heightmap_texture);

    glUseProgram(0);
  };
  NEVER_COPY(RPTerrainShader);
  RPTerrainShader(RPTerrainShader &&other)
      : m_shader{std::move(other.m_shader)},
        m_depth_shader{std::move(other.m_depth_shader)},
        m_depth_skirt_shader{std::move(other.m_depth_skirt_shader)},
        m_tile_config_ubo{std::move(other.m_tile_config_ubo)},
        m_noise_texture{other.m_noise_texture},
        m_depth_texture{other.m_depth_texture},
        m_heightmap_texture{other.m_heightmap_texture},
        m_material_block_binding{other.m_material_block_binding},
        m_tile_config_block_binding{other.m_tile_config_block_binding} {};
  void Begin() {
    m_shader.UseProgram();
    glGetBooleanv(GL_DEPTH_TEST, &g_depth_test);
    glGetBooleanv(GL_CULL_FACE, &g_cull_face);
    glGetIntegerv(GL_CULL_FACE_MODE, &g_cull_face_mode);
    glGetIntegerv(GL_FRONT_FACE, &g_front_face);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
  }
  void End() {
    if (g_depth_test == GL_FALSE)
      glDisable(GL_DEPTH_TEST);
    if (g_cull_face == GL_FALSE)
      glDisable(GL_CULL_FACE);
    glCullFace(g_cull_face_mode);
    glFrontFace(g_front_face);
    glUseProgram(0);
  }
  void BeginDepth() { m_depth_shader.UseProgram(); }
  void EndDepth() { glUseProgram(0); }
  void BeginDepthSkirt() { m_depth_skirt_shader.UseProgram(); }
  void EndDepthSkirt() { glUseProgram(0); }
  void SetUniforms(const glm::vec3 &camera_pos, const Light &light,
                   const TextureTileConfig &tileConfig, const glm::mat4 &mvp,
                   const glm::mat4 &light_mvp,
                   const glm::mat4 &model_matrix) const {
    m_shader.Uniform3fv("uCameraPos", camera_pos);
    m_shader.Uniform3fv("uAmbientLightColor", light.ambient_color);
    m_shader.Uniform3fv("uLightDir", light.direction);
    m_shader.Uniform3fv("uLightColor", light.diffuse_color);
    m_shader.UniformMatrix4fv("uMVP", GL_FALSE, mvp);
    m_shader.UniformMatrix4fv("uLightMVP", GL_FALSE, light_mvp);
    m_shader.UniformMatrix4fv("uModelMatrix", GL_FALSE, model_matrix);
    m_shader.Uniform1f("uSpecularPower", 64.0f);
    m_shader.Uniform1f("uShininessScale", 3000.0f);

    m_tile_config_ubo.BufferSubData(0, sizeof(tileConfig), &tileConfig);
    m_tile_config_ubo.BindBufferBase(m_tile_config_block_binding);
  }
  void SetDepthUniforms(const TextureTileConfig &tileConfig,
                        const glm::mat4 &mvp, const glm::mat4 &model_matrix) {
    m_depth_shader.UniformMatrix4fv("uMVP", GL_FALSE, mvp);
    m_depth_shader.UniformMatrix4fv("uModelMatrix", GL_FALSE, model_matrix);
    m_tile_config_ubo.BufferSubData(0, sizeof(tileConfig), &tileConfig);
    m_tile_config_ubo.BindBufferBase(m_tile_config_block_binding);
  };
  void setDepthSkirtUniforms(const TextureTileConfig &tileConfig,
                             const glm::mat4 &mvp) {
    m_depth_skirt_shader.UniformMatrix4fv("uMVP", GL_FALSE, mvp);
    m_tile_config_ubo.BufferSubData(0, sizeof(tileConfig), &tileConfig);
    m_tile_config_ubo.BindBufferBase(m_tile_config_block_binding);
  };
  void BindMaterialsBuffer(const GpuUBO &ubo) const {
    ubo.BindBufferBase(m_material_block_binding);
  }
  void BindDepthTexture(const GpuTexture &texture) const {
    BindTexture(texture, m_depth_texture);
  }
  void BindNoiseTexture(const GpuTexture &texture) const {
    BindTexture(texture, m_noise_texture);
  }
  void BindHeightmapTexture(const GpuTexture &texture) const {
    BindTexture(texture, m_heightmap_texture);
  }
  void BindBlendTexture(const GpuTexture &texture) const {
    Bind2DArrayTexture(texture, m_blend_texture);
  }

private:
  Shader m_shader;
  Shader m_depth_shader;
  Shader m_depth_skirt_shader;
  GpuUBO m_tile_config_ubo;
  const GLuint m_noise_texture{0};
  const GLuint m_depth_texture{1};
  const GLuint m_heightmap_texture{2};
  const GLuint m_blend_texture{3};
  const GLuint m_material_block_binding{0};
  const GLuint m_tile_config_block_binding{1};

  GLboolean g_depth_test, g_cull_face;
  GLint g_cull_face_mode, g_front_face;

  void BindTexture(const GpuTexture &texture,
                   const GLuint texture_location) const {
    glActiveTexture(GL_TEXTURE0 + texture_location);
    texture.BindTexture(GL_TEXTURE_2D);
  }
  void Bind2DArrayTexture(const GpuTexture &texture,
                          const GLuint texture_location) const {
    glActiveTexture(GL_TEXTURE0 + texture_location);
    texture.BindTexture(GL_TEXTURE_2D_ARRAY);
  }
};

class RPMaterial {
public:
  RPMaterial(const std::vector<Material> &materials,
             const std::vector<MaterialVertexData> &vertex_buffer_data,
             const std::vector<GLuint> &element_buffer_data);
  NEVER_COPY(RPMaterial);
  RPMaterial(RPMaterial &&other)
      : m_vao{std::move(other.m_vao)}, m_ubo{std::move(other.m_ubo)},
        m_vbo{std::move(other.m_vbo)}, m_ebo{std::move(other.m_ebo)},
        m_num_elements{other.m_num_elements} {};

  void DrawVertices() const;
  const GpuUBO &GetMaterialsBuffer() const { return m_ubo; };

private:
  GpuVAO m_vao;
  GpuUBO m_ubo;
  GpuVBO m_vbo;
  GpuEBO m_ebo;
  GLuint m_num_elements{0};
};

class RPTexturedMaterial {
  struct TexturedMaterialDrawCall {
    int firstTexture;
    int lastTexture;
    int firstVertex;
    int lastVertex;
  };

public:
  RPTexturedMaterial(const std::vector<std::string> &texture_files,
                     const std::vector<TextureVertexData> &vertex_buffer_data,
                     const std::vector<GLuint> &element_buffer_data,
                     const std::vector<MeshMap> &mesh_map);
  NEVER_COPY(RPTexturedMaterial);
  RPTexturedMaterial(RPTexturedMaterial &&other)
      : m_vao{std::move(other.m_vao)}, m_vbo{std::move(other.m_vbo)},
        m_ebo{std::move(other.m_ebo)}, m_textures{std::move(other.m_textures)},
        m_draw_calls{std::move(other.m_draw_calls)},
        m_num_elements{other.m_num_elements} {};

  void DrawVertices(const RPMaterialShader &shader) const;
  const std::vector<GpuTexture> &GetTextures() const { return m_textures; };

private:
  void PrecomputeDrawCalls(const std::vector<MeshMap> &mesh_map);
  GpuVAO m_vao;
  GpuVBO m_vbo;
  GpuEBO m_ebo;
  std::vector<GpuTexture> m_textures{};
  std::vector<TexturedMaterialDrawCall> m_draw_calls{};
  GLuint m_num_elements{0};
};

class RPDepthMap {
public:
  RPDepthMap(GLuint texture_size);
  NEVER_COPY(RPDepthMap);
  RPDepthMap(RPDepthMap &&other)
      : m_fbo{std::move(other.m_fbo)}, m_texture{std::move(other.m_texture)},
        m_texture_size{other.m_texture_size}, g_vp{other.g_vp},
        g_depth_test{other.g_depth_test}, g_cull_face{other.g_cull_face} {};
  glm::mat4 GetProjection(float fov, float near, float far) const;
  void Begin();
  void End();
  const GpuTexture &GetTexture() const;

private:
  GpuFBO m_fbo;
  GpuTexture m_texture;
  GLuint m_texture_size{0};
  glm::ivec4 g_vp{};
  GLboolean g_depth_test, g_cull_face;
  GLint g_cull_face_mode, g_front_face;
};

class RPTerrain {
public:
  RPTerrain();
  NEVER_COPY(RPTerrain);
  RPTerrain(RPTerrain &&other)
      : m_vao{std::move(other.m_vao)}, m_ubo{std::move(other.m_ubo)} {};
  void DrawVertices(int num_vertices) const;
  void DrawSkirt(int resolution) const;
  const GpuUBO &GetMaterialsBuffer() const { return m_ubo; };

private:
  GpuVAO m_vao;
  GpuUBO m_ubo;
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
  GpuVAO m_vao;
};

class RPTex {
public:
  RPTex();
  NEVER_COPY(RPTex);
  RPTex(RPTex &&other)
      : m_shader{std::move(other.m_shader)}, m_vao{std::move(other.m_vao)},
        m_vbo{std::move(other.m_vbo)},
        m_texture_binding{other.m_texture_binding} {};
  void Draw(const GpuTexture &texture) const;

private:
  Shader m_shader;
  GpuVAO m_vao;
  GpuVBO m_vbo;
  const GLuint m_texture_binding{1};
};