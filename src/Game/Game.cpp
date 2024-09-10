#include <imgui.h>
#include <iostream>
#include <random>

#include <PerlinNoise.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../Game.hpp"
#include "../MeshGroup.hpp"
#include "../Platform.hpp"
#include "../RenderPass.hpp"
#include "../utils.hpp"

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

static void HandleResize(const SDL_Event *event, Camera &camera) {
  int x = event->window.data1;
  int y = event->window.data2;
  glViewport(0, 0, x, y);
  camera.aspect_ratio = 1.0f * x / y;
}

void EnableAnisotropicFilter(const RPTexture &texture) {
  const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
  GLfloat maxAnisotropy = 0.0f;
  if (strstr(extensions, "GL_EXT_texture_filter_anisotropic")) {
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
                &maxAnisotropy); // Get the max anisotropy
    printf("x%.0f Anisotropic filtering is supported.\n", maxAnisotropy);
  } else {
    printf("Anisotropic filtering is not supported.\n");
    return;
  }
  typedef void (*glTexParameterfEXT_t)(GLenum target, GLenum pname,
                                       GLfloat param);
  glTexParameterfEXT_t glTexParameterfEXT =
      (glTexParameterfEXT_t)SDL_GL_GetProcAddress("glTexParameterfEXT");

  if (glTexParameterfEXT) {
    texture.BindTexture(GL_TEXTURE_2D);
    glTexParameterfEXT(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                       maxAnisotropy);
  } else {
    std::cout << "Anisotropic filtering function not available." << std::endl;
  }
}

RPTexture LoadTexture(const std::string &path) {
  RPTexture texture{};
  texture.BindTexture(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  EnableAnisotropicFilter(texture);

  int width, height, num_channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data =
      stbi_load(path.c_str(), &width, &height, &num_channels, 0);
  if (*data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);
  return texture;
}

RPTexture NoiseTexture(int texture_size, float x_scale, float y_scale) {
  RPTexture texture{};
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

RPTexture HeightmapTexture(int texture_size, const std::vector<float> &buffer) {
  RPTexture texture{};
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

RPTexture FaultFormationTexture(int texture_size, int gen_iterations,
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

std::vector<float> GenerateMidpointDisplacmentHeightMap(int texture_size) {
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
  int kSmoothIterations = 5;
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

RPTexture DisplacementTexture(int texture_size) {
  return HeightmapTexture(texture_size,
                          GenerateMidpointDisplacmentHeightMap(texture_size));
}

void RenderGui(const GameTimer &game_timer, Camera &camera, Light &light,
               TextureTileConfig &tileConfig, glm::mat4 &model_matrix) {

  ImGuiIO &io = ImGui::GetIO();
  ImGui::Begin("Performance Counters");
  ImGui::Text("evt=%luus", game_timer.evt_us);
  ImGui::Text("cpu=%luus", game_timer.cpu_us);
  ImGui::Text("gui=%luus", game_timer.gui_us);
  ImGui::Text("gpu=%luus", game_timer.gpu_us);
  ImGui::DragFloat4("camera.transform[3]", &camera.transform[3][0], .01f, -5.0f,
                    5.0f);
  ImGui::DragFloat4("uModelMatrix[3]", &model_matrix[3][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat3("camera.target", &camera.target[0], .01f, -10.0, 10.0f);
  ImGui::DragFloat3("light.direction", &light.direction[0], .01f, -10.0f,
                    10.0f);
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
                     100.0f);
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
                     1e-8f, 1e-3f, "%.8f");

  ImGui::Text("%.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
  ImGui::End();
}

int kDepthMapSize = 1024;
int kHeightMapSize = 1024;
int kNoiseTextureSize = 512;

void RegenerateTerrain(RPTexture &tex) {
  std::vector<float> heightmap_buffer{
      GenerateMidpointDisplacmentHeightMap(kHeightMapSize)};
  tex.BindTexture(GL_TEXTURE_2D);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kHeightMapSize, kHeightMapSize,
                  GL_RED, GL_FLOAT, &heightmap_buffer[0]);
  glGenerateMipmap(GL_TEXTURE_2D);
}

Game::Game(Platform *platform) : m_platform{platform} {
  // m_textures.emplace_back(
  // LoadTexture("assets/textures/eight_square_test/eight_square_test.png"));
  m_textures.emplace_back(
      LoadTexture("assets/textures/Poliigon_GrassPatchyGround_4585/2K/"
                  "Poliigon_GrassPatchyGround_4585_BaseColor.jpg"));
  m_textures.emplace_back(LoadTexture(
      "assets/textures/GroundDirtRocky020/GroundDirtRocky020_COL_2K.jpg"));
  m_textures.emplace_back(NoiseTexture(kNoiseTextureSize, 100.0f, 100.0f));
  m_textures.emplace_back(DisplacementTexture(kHeightMapSize));
  m_textures.emplace_back(LoadTexture("assets/textures/ogldev/water.png"));
  m_textures.emplace_back(
      LoadTexture("assets/textures/ogldev/tilable-IMG_0044-verydark.png"));
  m_textures.emplace_back(
      LoadTexture("assets/textures/ogldev/IMGP5497_seamless.jpg"));
  m_textures.emplace_back(
      LoadTexture("assets/textures/ogldev/IMGP5525_seamless.jpg"));
  m_mesh_groups.emplace_back(Import("assets/fullroom/fullroom.obj"));
  m_rp_material.emplace_back(m_mesh_groups[0].GetMaterials(),
                             m_mesh_groups[0].GetVertexBuffer(),
                             m_mesh_groups[0].GetElementBuffer());
  m_rp_depth_map.emplace_back(kDepthMapSize);
  m_rp_tex.emplace_back();
  m_rp_icon.emplace_back();
  m_rp_terrain.emplace_back();
  m_material_shader.emplace_back();
  m_terrain_shader.emplace_back();
  float kGridScale = 200.0f;
  m_light = {
      .ambient_color = {0.3f, 0.3f, 0.3f},
      .direction = {glm::normalize(glm::vec3(-0.2f, 0.2f, 0.2f))},
      .diffuse_color = {1.0f, 1.0f, 1.0f},
      .static_distance = M_SQRT2f32 * 2.0f * kGridScale,
      .static_fov = 30.0f,
  };
  glm::vec3 initial_camera_position{kGridScale / 2.0f, kGridScale / 2.0f,
                                    -kGridScale / 2.0f};
  glm::vec3 initial_camera_target{0.0, kGridScale / 8.0, 0.0};
  glm::mat4 transform{glm::lookAt(initial_camera_position,
                                  initial_camera_target, glm::vec3{0, 1, 0})};
  glm::vec2 drawable_size{m_platform->GetDrawableSize()};
  m_camera = {.transform = transform,
              .target = initial_camera_target,
              .aspect_ratio = 1.0f * drawable_size.x / drawable_size.y,
              .fov = 60,
              .near = 0.01f,
              .far = 1000.0f};
  m_model_matrix =
      glm::rotate(glm::mat4(1.0f), -1.0f, glm::vec3(0.0, 1.0, 0.0));
  m_terrain_matrix = glm::mat4(1.0f);
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
      .parallel_bias = 4e-4f,
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
      RegenerateTerrain(m_textures[3]);
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
  float kMovementSensitivity = 0.2f;
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
  // m_material_shader[0].BeginDepth();
  // m_material_shader[0].SetDepthUniforms(model_light_vp, model_light_vp,
  //                                       m_model_matrix);
  // m_rp_material[0].DrawVertices();
  // m_material_shader[0].EndDepth();

  // #2 terrain
  m_terrain_shader[0].BeginDepth();
  m_terrain_shader[0].BindHeightmapTexture(m_textures[3]);
  m_terrain_shader[0].SetDepthUniforms(m_tile_config, terrain_light_vp,
                                       m_model_matrix);
  m_rp_terrain[0].DrawVertices(m_tile_config.resolution);
  m_terrain_shader[0].EndDepth();

  // End Shadow Pass
  m_rp_depth_map[0].End();

  // Draw Material
  // m_material_shader[0].Begin();
  // m_material_shader[0].BindDepthTexture(m_rp_depth_map[0].GetTexture());
  // m_material_shader[0].BindDiffuseTexture(m_textures[0]);
  // m_material_shader[0].BindNoiseTexture(m_textures[2]);
  // m_material_shader[0].BindMaterialsBuffer(
  //     m_rp_material[0].GetMaterialsBuffer());
  // m_material_shader[0].SetUniforms(camera_position, m_light, model_vp,
  //                                  model_light_vp, m_model_matrix);
  // m_rp_material[0].DrawVertices();
  // m_material_shader[0].End();

  // Draw Terrain
  m_terrain_shader[0].Begin();
  m_terrain_shader[0].BindDiffuseTexture(m_textures[0]);
  m_terrain_shader[0].BindBlendTexture(m_textures[1]);
  m_terrain_shader[0].BindNoiseTexture(m_textures[2]);
  m_terrain_shader[0].BindHeightmapTexture(m_textures[3]);
  m_terrain_shader[0].BindVeryHighTexture(m_textures[4]);
  m_terrain_shader[0].BindHighTexture(m_textures[5]);
  m_terrain_shader[0].BindMediumTexture(m_textures[6]);
  m_terrain_shader[0].BindLowTexture(m_textures[7]);
  m_terrain_shader[0].BindDepthTexture(m_rp_depth_map[0].GetTexture());
  m_terrain_shader[0].SetUniforms(camera_position, m_light, m_tile_config,
                                  terrain_vp, terrain_light_vp,
                                  m_terrain_matrix);
  m_terrain_shader[0].BindMaterialsBuffer(m_rp_terrain[0].GetMaterialsBuffer());
  m_rp_terrain[0].DrawVertices(m_tile_config.resolution);
  m_terrain_shader[0].End();

  // draw shadow map to screen
  // m_rp_tex[0].Draw(m_rp_depth_map[0].GetTexture());

  // 3d icons
  m_rp_icon[0].Draw(vp * glm::vec4(static_light_pos, 1.0),
                    glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  m_rp_icon[0].Draw(vp * glm::vec4(m_camera.target, 1.0),
                    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  m_game_timer.t_finish_draw_calls = SDL_GetPerformanceCounter();
  RenderGui(m_game_timer, m_camera, m_light, m_tile_config, m_model_matrix);
  m_game_timer.t_finish_gui_draw = SDL_GetPerformanceCounter();
  glFinish();
  m_game_timer.t_finish_render = SDL_GetPerformanceCounter();
}