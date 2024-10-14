#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <random>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION
#include <PerlinNoise.hpp>

#include "../BlpLoader.hpp"
#include "../Game.hpp"
#include "../GpuTexture.hpp"
#include "../ImageData.hpp"
#include "../MeshGroup.hpp"
#include "../Platform.hpp"
#include "../RPWowTerrain.hpp"
#include "../RenderPass.hpp"
#include "../WowItemLookup.hpp"
#include "../utils.hpp"

static void HandleResize(const SDL_Event *event, Camera &camera) {
  int x = event->window.data1;
  int y = event->window.data2;
  glViewport(0, 0, x, y);
  camera.aspect_ratio = float(x) / float(y);
}

GpuTexture NoiseTexture(int texture_size, float x_scale, float y_scale) {
  GpuTexture texture{};
  texture.BindTexture(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width = texture_size;
  int height = texture_size;
  float inverse_width = 1.0 / width;
  float inverse_height = 1.0 / height;
  std::vector<float> noise_buffer(width * height);
  const siv::PerlinNoise::seed_type seed = 234567u;
  const siv::PerlinNoise perlin{seed};
  for (int i = 0; i < height; i++) {
    float x_frac = i * inverse_height;
    int rowOffset = i * width;
    for (int j = 0; j < width; j++) {
      float y_frac = j * inverse_width;
      noise_buffer[rowOffset + j] =
          perlin.noise2D_01(x_frac * x_scale, y_frac * y_scale);
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT,
               &noise_buffer[0]);
  glGenerateMipmap(GL_TEXTURE_2D);
  return texture;
}

void MapToRange(std::vector<float> &buffer, float min_range, float max_range) {
  float minVal = buffer[0];
  float maxVal = buffer[0];
  for (float &val : buffer) {
    if (val < minVal) {
      minVal = val;
    } else if (val > maxVal) {
      maxVal = val;
    }
  }
  float in_range = maxVal - minVal;
  float out_range = max_range - min_range;
  for (float &val : buffer) {
    float frac = (val - minVal) / in_range;
    val = frac * out_range + min_range;
  }
}

float Lerp(float x1, float x2, float factor) {
  return factor * x1 + (1.0f - factor) * x2;
}

enum FILTER_DIRECTION { FD_UP, FD_DOWN, FD_LEFT, FD_RIGHT } filterDirection;

void FIRFilter(std::vector<float> &buffer, float factor,
               FILTER_DIRECTION direction, int height, int width) {
  switch (direction) {
  case (FD_RIGHT): {
    for (int i = 0; i < height; i++) {
      int rowOffset = i * width;
      for (int j = 1; j < width; j++) {
        buffer[j + rowOffset] =
            Lerp(buffer[(j - 1) + rowOffset], buffer[j + rowOffset], factor);
      }
    }
    break;
  }
  case (FD_LEFT): {
    for (int i = 0; i < height; i++) {
      int rowOffset = i * width;
      for (int j = width - 2; j >= 0; j--) {
        buffer[j + rowOffset] =
            Lerp(buffer[(j + 1) + rowOffset], buffer[j + rowOffset], factor);
      }
    }
    break;
  }
  case (FD_UP): {
    for (int j = 0; j < width; j++) {
      for (int i = 1; i < height; i++) {
        int rowOffset = i * width;
        int prevRowOffset = (i - 1) * width;
        buffer[j + rowOffset] =
            Lerp(buffer[j + prevRowOffset], buffer[j + rowOffset], factor);
      }
    }
    break;
  }
  case (FD_DOWN): {
    for (int j = 0; j < width; j++) {
      for (int i = height - 2; i >= 0; i--) {
        int rowOffset = i * width;
        int prevRowOffset = (i + 1) * width;
        buffer[j + rowOffset] =
            Lerp(buffer[j + prevRowOffset], buffer[j + rowOffset], factor);
      }
    }
    break;
  }
  }
}

void FaultFormation(std::vector<float> &buffer, int texture_size,
                    int gen_iterations) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, texture_size - 1);

  for (int i = 0; i < gen_iterations; i++) {
    float i_frac = float(i) / float(gen_iterations);
    float height = 1.0 - i_frac;
    glm::ivec2 p1{distrib(gen), distrib(gen)};
    glm::ivec2 p2{distrib(gen), distrib(gen)};
    glm::ivec2 dir{p2 - p1};
    for (int y = 0; y < texture_size; y++) {
      for (int x = 0; x < texture_size; x++) {
        glm::ivec2 dir_in{x - p1.x, y - p1.y};
        int cross_product = dir_in.x * dir.y - dir.x * dir_in.y;
        if (cross_product > 0) {
          buffer[x + y * texture_size] += height;
        }
      }
    }
  }
}

std::vector<float> GenerateFaultFormationHeightMap(int texture_size,
                                                   int gen_iterations,
                                                   int smooth_iterations,
                                                   float smooth_factor) {
  std::vector<float> heightmap_buffer(texture_size * texture_size);

  // FaultFormation
  FaultFormation(heightmap_buffer, texture_size, gen_iterations);

  // Smooth
  for (int i = 0; i < smooth_iterations; i++) {
    FIRFilter(heightmap_buffer, smooth_factor, FD_UP, texture_size,
              texture_size);
    FIRFilter(heightmap_buffer, smooth_factor, FD_DOWN, texture_size,
              texture_size);
    FIRFilter(heightmap_buffer, smooth_factor, FD_LEFT, texture_size,
              texture_size);
    FIRFilter(heightmap_buffer, smooth_factor, FD_RIGHT, texture_size,
              texture_size);
  }

  // Normalize
  MapToRange(heightmap_buffer, 0, 1.0);

  return heightmap_buffer;
}

GpuTexture HeightmapTexture(int texture_size,
                            const std::vector<float> &buffer) {
  GpuTexture texture{};
  texture.BindTexture(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, texture_size, texture_size, 0, GL_RED,
               GL_FLOAT, &buffer[0]);
  glGenerateMipmap(GL_TEXTURE_2D);
  return texture;
}

GpuTexture FaultFormationTexture(int texture_size, int gen_iterations,
                                 int smooth_iterations, float smooth_factor) {
  std::vector<float> fault_formation_buffer{GenerateFaultFormationHeightMap(
      texture_size, gen_iterations, smooth_iterations, smooth_factor)};
  return HeightmapTexture(texture_size, fault_formation_buffer);
};

void DiamondStep(std::vector<float> &buffer, int texture_size, int rect_size,
                 float cur_height) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> distrib(-cur_height, cur_height);
  int half_rect_size = rect_size / 2;
  for (int y = 0; y < texture_size; y += rect_size) {
    for (int x = 0; x < texture_size; x += rect_size) {
      int next_x = (x + rect_size) % texture_size;
      int next_y = (y + rect_size) % texture_size;

      if (next_x < x) {
        next_x = texture_size - 1;
      }
      if (next_y < y) {
        next_y = texture_size - 1;
      }
      float top_left = buffer[x + texture_size * y];
      float top_right = buffer[next_x + texture_size * y];
      float bottom_left = buffer[x + texture_size * next_y];
      float bottom_right = buffer[next_x + texture_size * next_y];

      int mid_x = (x + half_rect_size) % texture_size;
      int mid_y = (y + half_rect_size) % texture_size;

      float rand_value = distrib(gen);
      float mid_point =
          (top_left + top_right + bottom_left + bottom_right) / 4.0f;
      buffer[mid_x + texture_size * mid_y] = mid_point + rand_value;
    }
  }
}
void SquareStep(std::vector<float> &buffer, int texture_size, int rect_size,
                float cur_height) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> distrib(-cur_height, cur_height);
  int half_rect_size = rect_size / 2;

  for (int y = 0; y < texture_size; y += rect_size) {
    for (int x = 0; x < texture_size; x += rect_size) {
      int next_x = (x + rect_size) % texture_size;
      int next_y = (y + rect_size) % texture_size;

      if (next_x < x) {
        next_x = texture_size - 1;
      }

      if (next_y < y) {
        next_y = texture_size - 1;
      }

      int mid_x = (x + half_rect_size) % texture_size;
      int mid_y = (y + half_rect_size) % texture_size;

      int prev_mid_x = (x - half_rect_size + texture_size) % texture_size;
      int prev_mid_y = (y - half_rect_size + texture_size) % texture_size;

      float cur_top_left = buffer[x + texture_size * y];
      float cur_top_right = buffer[next_x + texture_size * y];
      float cur_center = buffer[mid_x + texture_size * mid_y];
      float prev_y_center = buffer[mid_x + texture_size * prev_mid_y];
      float cur_bot_left = buffer[x + texture_size * next_y];
      float prev_x_center = buffer[prev_mid_x + texture_size * mid_y];

      float cur_left_mid =
          (cur_top_left + cur_center + cur_bot_left + prev_x_center) / 4.0f +
          distrib(gen);
      float cur_top_mid =
          (cur_top_left + cur_center + cur_top_right + prev_y_center) / 4.0f +
          distrib(gen);

      buffer[mid_x + texture_size * y] = cur_top_mid;
      buffer[x + texture_size * mid_y] = cur_left_mid;
    }
  }
}

std::vector<float> GenerateMidpointDisplacementHeightMap(int texture_size) {
  std::vector<float> buffer(texture_size * texture_size);
  float kRoughness = 1.0f;
  int rect_size = texture_size;
  float cur_height = rect_size / 2.0f;
  float height_reduce = pow(2.0f, -kRoughness);

  while (rect_size > 0) {
    // Diamond Step
    DiamondStep(buffer, texture_size, rect_size, cur_height);
    // Square Step
    SquareStep(buffer, texture_size, rect_size, cur_height);

    rect_size /= 2;
    cur_height *= height_reduce;
  }

  // Smooth
  int kSmoothIterations = 1;
  float kSmoothFactor = 0.5;
  for (int i = 0; i < kSmoothIterations; i++) {
    FIRFilter(buffer, kSmoothFactor, FD_UP, texture_size, texture_size);
    FIRFilter(buffer, kSmoothFactor, FD_DOWN, texture_size, texture_size);
    FIRFilter(buffer, kSmoothFactor, FD_LEFT, texture_size, texture_size);
    FIRFilter(buffer, kSmoothFactor, FD_RIGHT, texture_size, texture_size);
  }

  MapToRange(buffer, 0, 1.0);
  return buffer;
};

GpuTexture DisplacementTexture(int texture_size) {
  return HeightmapTexture(texture_size,
                          GenerateMidpointDisplacementHeightMap(texture_size));
}

std::vector<float>
ConvertMapTileToHeightmap(const std::vector<TextureVertexData> &verts,
                          int tilesize) {
  glm::vec3 pz = verts[0].position;
  float x_min = pz.x;
  float x_max = pz.x;
  float y_min = pz.y;
  float y_max = pz.y;
  float z_min = pz.z;
  float z_max = pz.z;
  for (const auto &v : verts) {
    glm::vec3 p{v.position};
    x_min = std::min(x_min, p.x);
    x_max = std::max(x_max, p.x);
    y_min = std::min(y_min, p.y);
    y_max = std::max(y_max, p.y);
    z_min = std::min(z_min, p.z);
    z_max = std::max(z_max, p.z);
  }
  std::vector<float> heightmap(tilesize * tilesize);
  for (const auto &v : verts) {
    glm::vec3 p{v.position};
    float x_frac = (p.x - x_min) / (x_max - x_min);
    float y_frac = (p.y - y_min) / (y_max - y_min);
    float z_frac = 1.0 - ((p.z - z_min) / (z_max - z_min)); // invert
    int x_grid = round(x_frac * (float(tilesize) - 1.0));
    int z_grid = round(z_frac * (float(tilesize) - 1.0));
    heightmap[z_grid * tilesize + x_grid] = y_frac;
  }
  return heightmap;
}

void RenderGui(const GameTimer &game_timer, Camera &camera, Light &light,
               TextureTileConfig &tileConfig, glm::mat4 &model_matrix) {

  ImGuiIO &io = ImGui::GetIO();
  ImGui::Begin("Performance Counters");
  ImGui::Text("evt=%luus", game_timer.evt_us);
  ImGui::Text("cpu=%luus", game_timer.cpu_us);
  ImGui::Text("gui=%luus", game_timer.gui_us);
  ImGui::Text("gpu=%luus", game_timer.gpu_us);
  ImGui::DragFloat4("camera.transform[3]", &camera.transform[3][0], .1f,
                    -100.0f, 100.0f);
  ImGui::DragFloat4("uModelMatrix[3]", &model_matrix[3][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat3("camera.target", &camera.target[0], .01f, -10.0, 10.0f);
  ImGui::DragFloat3("light.direction", &light.direction[0], .01f, -10.0f,
                    10.0f);
  ImGui::DragFloat3("light.ambient_color", &light.ambient_color[0], .1f, 0.0f,
                    1.0f);
  ImGui::DragFloat("light.static_distance", &light.static_distance, .01f, 1.0f,
                   1000.0f);
  ImGui::DragFloat("light.static_fov", &light.static_fov, .01f, 1.0f, 120.0f);
  ImGui::DragFloat("camera.fov", &camera.fov, 1.0f, 0.0f, 120.0f);
  ImGui::DragFloat("camera.near", &camera.near, 0.001f, 0.001f, 1.0f);
  ImGui::DragFloat("camera.far", &camera.far, 0.1f, 1.0f, 1000.0f);

  ImGui::SliderFloat("tileConfig.height_scale", &tileConfig.height_scale, 0.0f,
                     1000.0f);
  ImGui::SliderFloat("tileConfig.width_scale", &tileConfig.width_scale, 0.0f,
                     1.0f);
  ImGui::SliderFloat("tileConfig.grid_scale", &tileConfig.grid_scale, 1.0f,
                     1000.0f);
  ImGui::SliderInt("tileConfig.resolution", &tileConfig.resolution, 32, 1024);
  ImGui::SliderFloat("tileConfig.repeat_scale", &tileConfig.repeat_scale, 0.0f,
                     1000.0f);
  ImGui::SliderFloat("tileConfig.rotation_scale", &tileConfig.rotation_scale,
                     0.0f, 300.0f);
  ImGui::SliderFloat("tileConfig.translation_scale",
                     &tileConfig.translation_scale, 0.0f, 50.0f);
  ImGui::SliderFloat("tileConfig.noise_scale", &tileConfig.noise_scale, 0.0f,
                     1.0f);
  ImGui::SliderFloat("tileConfig.hue_scale", &tileConfig.hue_scale, 0.0f, 1.0f);
  ImGui::SliderFloat("tileConfig.saturation_scale",
                     &tileConfig.saturation_scale, 0.0f, 10.0f);
  ImGui::SliderFloat("tileConfig.brightness_scale",
                     &tileConfig.brightness_scale, 0.0f, 10.0f);
  ImGui::SliderFloat("tileConfig.flat_bias", &tileConfig.flat_bias, 1e-8f,
                     1e-3f, "%.8f");
  ImGui::SliderFloat("tileConfig.parallel_bias", &tileConfig.parallel_bias,
                     1e-4f, 1e-2f, "%.8f");

  ImGui::Text("%.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
  ImGui::End();
}

constexpr int kDepthMapSize = 1024;
constexpr int kHeightMapSize = 256;
constexpr int kNoiseTextureSize = 256;

void RegenerateTerrain(GpuTexture &tex) {
  std::vector<float> heightmap_buffer{
      GenerateMidpointDisplacementHeightMap(kHeightMapSize)};
  tex.BindTexture(GL_TEXTURE_2D);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kHeightMapSize, kHeightMapSize,
                  GL_RED, GL_FLOAT, &heightmap_buffer[0]);
  glGenerateMipmap(GL_TEXTURE_2D);
}

Game::Game(Platform *platform) : m_platform{platform} {
  InitWowItemLookup();

  m_material_shader.emplace_back();
  m_terrain_shader.emplace_back();
  m_wow_terrain_shader.emplace_back();
  m_rp_tex.emplace_back();
  m_rp_icon.emplace_back();

  m_textures.emplace_back(NoiseTexture(kNoiseTextureSize, 100.0f, 100.0f));
  m_textures.emplace_back(DisplacementTexture(kHeightMapSize));

  stbi_set_flip_vertically_on_load(true);
  m_textures.emplace_back(GpuTexture::FromPaths({
      "assets/textures/veryhigh/snow_01_diff_1k.jpg",
      "assets/textures/high/sparse_grass_diff_1k.jpg",
      "assets/textures/medium/rocky_terrain_diff_1k.jpg",
      "assets/textures/low/gray_rocks_diff_1k.jpg",
  }));
  m_mesh_groups.emplace_back(Import("assets/fullroom/fullroom.obj"));
  m_rp_material.emplace_back(m_mesh_groups[0].GetMaterials(),
                             m_mesh_groups[0].GetMaterialVertexBuffer(),
                             m_mesh_groups[0].GetElementBuffer());

  m_mesh_groups.emplace_back(
      Import("/home/nick/wow.export/maps/azeroth/adt_29_52.obj"));
  m_rp_textured_material.emplace_back(m_mesh_groups[1].GetTextureFiles(),
                                      m_mesh_groups[1].GetTextureVertexBuffer(),
                                      m_mesh_groups[1].GetElementBuffer(),
                                      m_mesh_groups[1].GetMeshMap());

  m_rp_depth_map.emplace_back(kDepthMapSize);
  m_rp_terrain.emplace_back();
  std::vector<std::pair<std::string, std::string>> wow_map_paths{};
  for (int i = 29; i <= 35; i++) {
    for (int j = 35; j <= 52; j++) {
      std::pair p{"/home/nick/wow.export/maps/azeroth/azeroth.wdt",
                  "azeroth_" + std::to_string(i) + "_" + std::to_string(j)};
      if (std::filesystem::exists("/home/nick/wow.export/maps/azeroth/" +
                                  p.second + ".adt")) {
        wow_map_paths.push_back(p);
      }
    }
  }
  std::unordered_map<uint32_t, int> wow_file_data_id_texture_map;
  GpuTexParameters gtp{{GL_TEXTURE_WRAP_S, GL_REPEAT},
                       {GL_TEXTURE_WRAP_T, GL_REPEAT},
                       {GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
                       {GL_TEXTURE_MAG_FILTER, GL_LINEAR}};
  int kTextureArraySize = 1024;
  TextureArray terrain_texture_array{256, 256, 4, kTextureArraySize, gtp};
  for (const auto &p : wow_map_paths) {
    m_rp_wow_terrain.emplace_back(p.first, p.second, terrain_texture_array,
                                  wow_file_data_id_texture_map);
  }
  terrain_texture_array.GetTexture().GenerateMipmap(GL_TEXTURE_2D_ARRAY);
  m_texture_arrays.push_back(std::move(terrain_texture_array));

  float kGridScale = 200.0f;
  m_light = {
      .ambient_color = {0.5f, 0.5f, 0.5f},
      .direction = {glm::normalize(glm::vec3(-0.2f, 0.2f, 0.2f))},
      .diffuse_color = {1.0f, 1.0f, 1.0f},
      .static_distance = M_SQRT2f32 * kGridScale,
      .static_fov = 60.0f,
  };
  glm::vec3 initial_camera_position{-11000.0, 75.0, -1300.0};
  glm::vec3 initial_camera_target{0.0, 0.0, 0.0};
  glm::mat4 transform{glm::lookAt(initial_camera_position,
                                  initial_camera_target, glm::vec3{0, 1, 0})};
  glm::vec2 drawable_size{m_platform->GetDrawableSize()};
  m_camera = {.transform = transform,
              .target = initial_camera_target,
              .aspect_ratio = 1.0f * drawable_size.x / drawable_size.y,
              .fov = 60,
              .near = 0.1f,
              .far = 10000.0f};
  m_model_matrix = glm::rotate(glm::mat4(1.0), M_PI_2f32, glm::vec3(0, 1, 0));
  m_model_matrix = glm::translate(m_model_matrix, glm::vec3(0, -100, 0));
  m_terrain_matrix = glm::mat4(1.0);
  m_tile_config = {
      .height_scale = kGridScale / 4.0f,
      .width_scale = 1.0f,
      .grid_scale = kGridScale,
      .resolution = 512,
      .repeat_scale = kGridScale / 25.0f,
      .rotation_scale = 100.0f,
      .translation_scale = 10.0f,
      .noise_scale = 1.0f,
      .hue_scale = 0.1f,
      .saturation_scale = 0.1f,
      .brightness_scale = 0.2f,
      .flat_bias = 2e-4f,
      .parallel_bias = 2e-3f,
  };
  m_game_timer.count_per_microsecond =
      SDL_GetPerformanceFrequency() / 1'000'000;
}
void Game::BeginFrame() {
  m_game_timer.evt_us =
      (m_game_timer.t_finish_events - m_game_timer.t_frame_start) /
      (m_game_timer.count_per_microsecond);
  m_game_timer.cpu_us =
      (m_game_timer.t_finish_draw_calls - m_game_timer.t_finish_events) /
      (m_game_timer.count_per_microsecond);
  m_game_timer.gui_us =
      (m_game_timer.t_finish_gui_draw - m_game_timer.t_finish_draw_calls) /
      (m_game_timer.count_per_microsecond);
  m_game_timer.gpu_us =
      (m_game_timer.t_finish_render - m_game_timer.t_finish_gui_draw) /
      (m_game_timer.count_per_microsecond);
  m_game_timer.t_frame_start = SDL_GetPerformanceCounter();
}
void Game::Event(const SDL_Event &event) {
  switch (event.type) {
  case SDL_QUIT:
    m_platform->QueueQuit();
    break;
  case SDL_KEYDOWN: {
    switch (event.key.keysym.sym) {
    case SDLK_q: {
      m_platform->QueueQuit();
      break;
    }
    case SDLK_r: {
      RegenerateTerrain(m_textures[1]);
      break;
    }
    }
    break;
  }
  case SDL_MOUSEWHEEL: {
    float kZoomSensitivity = 0.2f;
    MoveAlongCameraAxes(
        m_camera.transform,
        glm::vec3(0, 0, 1.0f * event.motion.x * kZoomSensitivity));
    break;
  }
  case SDL_WINDOWEVENT:
    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
      HandleResize(&event, m_camera);
    }
    break;
  }
}
void HandleInput(Camera &camera) {
  float kMovementSensitivity = 5.0f;
  float kMouseMovementSensitivity = 0.005f;
  float kMouseLookSensitivity = .005f;
  int x, y, l;
  Uint32 mouse_buttons = SDL_GetRelativeMouseState(&x, &y);
  const Uint8 *keyboard_buttons = SDL_GetKeyboardState(&l);
  glm::vec3 movement{0.0};
  if (keyboard_buttons[SDL_SCANCODE_W]) {
    movement.z += 1;
  }
  if (keyboard_buttons[SDL_SCANCODE_A]) {
    movement.x -= 1;
  }
  if (keyboard_buttons[SDL_SCANCODE_S]) {
    movement.z -= 1;
  }
  if (keyboard_buttons[SDL_SCANCODE_D]) {
    movement.x += 1;
  }
  if (keyboard_buttons[SDL_SCANCODE_R]) {
    movement.y += 1;
  }
  if (keyboard_buttons[SDL_SCANCODE_F]) {
    movement.y -= 1;
  }
  if (movement != glm::vec3{0.0}) {
    movement = glm::normalize(movement);
    MoveAlongCameraAxes(camera.transform, movement * kMovementSensitivity);
  }
  if (x != 0 || y != 0) {
    float amount_right = 1.0 * x;
    float amount_up = -1.0 * y;
    if (mouse_buttons & SDL_BUTTON_MMASK) {
      if (keyboard_buttons[SDL_SCANCODE_LSHIFT]) {
        SlideViewWithTarget(camera.transform, camera.target,
                            amount_right * kMouseMovementSensitivity,
                            amount_up * kMouseMovementSensitivity);
      } else {
        if (y != 0) {
          OrbitPitch(camera.transform, camera.target,
                     amount_up * kMouseLookSensitivity);
        }
        if (x != 0) {
          OrbitYaw(camera.transform, camera.target,
                   -amount_right * kMouseLookSensitivity);
        }
      }
    }
    if (mouse_buttons & SDL_BUTTON_RMASK) {
      if (y != 0) {
        RotatePitch(camera.transform, amount_up * kMouseLookSensitivity);
      }
      if (x != 0) {
        RotateYaw(camera.transform, -amount_right * kMouseLookSensitivity);
      }
    }
  }
}

void Game::Render() {
  HandleInput(m_camera);
  m_game_timer.t_finish_events = SDL_GetPerformanceCounter();
  glClear(GL_DEPTH_BUFFER_BIT);
  static const float bg[] = {0.2f, 0.2f, 0.2f, 1.0f};
  glClearBufferfv(GL_COLOR, 0, bg);

  glm::vec3 camera_position{GetCameraPos(m_camera.transform)};
  glm::mat4 camera_projection =
      glm::perspective(glm::radians(m_camera.fov), m_camera.aspect_ratio,
                       m_camera.near, m_camera.far);
  glm::mat4 vp = camera_projection * m_camera.transform;
  glm::mat4 model_vp = vp * m_model_matrix;
  glm::mat4 terrain_vp = vp * m_terrain_matrix;
  glm::mat4 wow_terrain_vp = vp;

  glm::vec3 static_light_pos{glm::normalize(m_light.direction) *
                             m_light.static_distance};
  glm::vec3 yAxis{0.0, 1.0, 0.0};
  glm::mat4 light_transform =
      glm::lookAt(static_light_pos, m_camera.target, yAxis);
  glm::mat4 light_projection = m_rp_depth_map[0].GetProjection(
      m_light.static_fov, 0.1f, m_light.static_distance * 2.0);
  glm::mat4 light_vp = light_projection * light_transform;
  glm::mat4 model_light_vp = light_vp * m_model_matrix;
  glm::mat4 terrain_light_vp = light_vp * m_terrain_matrix;

  // Shadow Map Pass
  m_rp_depth_map[0].Begin();

  // #1 models
  m_material_shader[0].BeginDepth();
  m_material_shader[0].SetDepthUniforms(model_light_vp, model_light_vp,
                                        m_model_matrix);
  m_rp_material[0].DrawVertices();
  for (auto &rp : m_rp_textured_material) {
    rp.DrawVertices(m_material_shader[0]);
  }
  m_material_shader[0].EndDepth();

  // #2 terrain
  // m_terrain_shader[0].BindHeightmapTexture(m_textures[1]);
  // m_terrain_shader[0].SetDepthUniforms(m_tile_config, terrain_light_vp,
  //                                      m_model_matrix);
  // m_terrain_shader[0].BeginDepth();
  // m_rp_terrain[0].DrawVertices(m_tile_config.resolution);
  // m_terrain_shader[0].EndDepth();
  // m_terrain_shader[0].setDepthSkirtUniforms(m_tile_config,
  // terrain_light_vp); m_terrain_shader[0].BeginDepthSkirt();
  // m_rp_terrain[0].DrawSkirt(m_tile_config.resolution);
  // m_terrain_shader[0].EndDepthSkirt();

  // End Shadow Pass
  m_rp_depth_map[0].End();

  // Draw Material
  m_material_shader[0].BindDepthTexture(m_rp_depth_map[0].GetTexture());
  m_material_shader[0].BindTextures(m_textures.begin(), m_textures.begin() + 1);
  m_material_shader[0].BindMaterialsBuffer(
      m_rp_material[0].GetMaterialsBuffer());
  m_material_shader[0].SetUniforms(camera_position, m_light, model_vp,
                                   model_light_vp, m_model_matrix);
  m_material_shader[0].Begin();
  // m_rp_material[0].DrawVertices();
  for (auto &rp : m_rp_textured_material) {
    rp.DrawVertices(m_material_shader[0]);
  }
  m_material_shader[0].End();

  // Draw Terrain
  m_terrain_shader[0].BindMaterialsBuffer(m_rp_terrain[0].GetMaterialsBuffer());
  m_terrain_shader[0].BindNoiseTexture(m_textures[0]);
  m_terrain_shader[0].BindHeightmapTexture(m_textures[1]);
  m_terrain_shader[0].BindBlendTexture(m_textures[2]);
  m_terrain_shader[0].BindDepthTexture(m_rp_depth_map[0].GetTexture());
  m_terrain_shader[0].SetUniforms(camera_position, m_light, m_tile_config,
                                  terrain_vp, terrain_light_vp,
                                  m_terrain_matrix);
  m_terrain_shader[0].Begin();
  // m_rp_terrain[0].DrawVertices(m_tile_config.resolution);
  m_terrain_shader[0].End();

  // Draw Wow Terrain
  m_wow_terrain_shader[0].Begin();
  for (const auto &t : m_rp_wow_terrain) {
    m_wow_terrain_shader[0].BindBlendTexture(m_texture_arrays[0].GetTexture());
    m_wow_terrain_shader[0].BindAlphaTexture(t.GetAlphaTexture());
    m_wow_terrain_shader[0].BindShadowTexture(t.GetShadowTexture());
    m_wow_terrain_shader[0].BindHeightmapBuffer(t.GetHeightmapSSBO());
    m_wow_terrain_shader[0].BindTextureSlotsBuffer(t.GetTextureSlotsSSBO());
    m_wow_terrain_shader[0].BindAlphaSlotsBuffer(t.GetAlphaSlotsSSBO());
    m_wow_terrain_shader[0].BindHolesBuffer(t.GetHolesSSBO());
    m_wow_terrain_shader[0].SetUniforms(camera_position, m_light,
                                        wow_terrain_vp, t.GetCornerPosition());
    t.DrawVertices();
  }
  m_wow_terrain_shader[0].End();

  // draw shadow map to screen
  // m_rp_tex[0].Draw(m_textures[m_textures.size() - 1]);

  // 3d icons
  m_rp_icon[0].Draw(vp * glm::vec4(static_light_pos, 1.0),
                    glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  m_rp_icon[0].Draw(vp * glm::vec4(m_camera.target, 1.0),
                    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  m_game_timer.t_finish_draw_calls = SDL_GetPerformanceCounter();
  RenderGui(m_game_timer, m_camera, m_light, m_tile_config, m_model_matrix);
  m_game_timer.t_finish_gui_draw = SDL_GetPerformanceCounter();
  m_game_timer.t_finish_render = SDL_GetPerformanceCounter();
}