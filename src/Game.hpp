#pragma once

#include "MeshGroup.hpp"
#include "MultiTextureArray.hpp"
#include "RPWowTerrain.hpp"
#include "RenderPass.hpp"
#include <SDL.h>

struct GameTimer {
  Uint64 t_frame_start{0};
  Uint64 t_finish_events{0};
  Uint64 t_finish_draw_calls{0};
  Uint64 t_finish_gui_draw{0};
  Uint64 t_finish_render{0};
  Uint64 count_per_microsecond{0};
  Uint64 evt_us{0};
  Uint64 cpu_us{0};
  Uint64 gui_us{0};
  Uint64 gpu_us{0};
};

class Platform;
class Game {
public:
  Game(Platform *platform);
  void BeginFrame();
  void Render();
  void Event(const SDL_Event &event);

private:
  Platform *m_platform;
  Light m_light;
  Camera m_camera;
  glm::mat4 m_model_matrix;
  glm::mat4 m_terrain_matrix;
  TextureTileConfig m_tile_config;
  std::vector<MeshGroup> m_mesh_groups{};
  std::vector<RPMaterial> m_rp_material{};
  std::vector<RPTexturedMaterial> m_rp_textured_material{};
  std::vector<RPDepthMap> m_rp_depth_map{};
  std::vector<RPTex> m_rp_tex{};
  std::vector<RPIcon> m_rp_icon{};
  std::vector<RPTerrain> m_rp_terrain{};
  std::vector<RPWowTerrain> m_rp_wow_terrain{};
  std::vector<RPMaterialShader> m_material_shader{};
  std::vector<RPTerrainShader> m_terrain_shader{};
  std::vector<RPWowTerrainShader> m_wow_terrain_shader{};
  std::vector<GpuTexture> m_textures{};
  std::vector<MultiTextureArray> m_multi_texture_arrays{};
  GameTimer m_game_timer{};
};