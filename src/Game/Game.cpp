#include <imgui.h>
#include <iostream>

#include "../Game.hpp"
#include "../MeshGroup.hpp"
#include "../Platform.hpp"
#include "../RenderPass.hpp"
#include "../utils.hpp"

static void HandleResize(const SDL_Event *event, Camera &camera) {
  int x = event->window.data1;
  int y = event->window.data2;
  glViewport(0, 0, x, y);
  camera.aspect_ratio = 1.0f * x / y;
}

void RenderGui(const GameTimer &game_timer, Camera &camera, Light &light,
               glm::mat4 &model_matrix) {

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
  // ImGui::DragFloat3("light.uLightPosition", &light.position[0], .01f,
  // -10.0f, 10.0f);
  ImGui::DragFloat3("light.uLightDir", &light.direction[0], .01f, -10.0f,
                    10.0f);
  ImGui::DragFloat("camera.fov", &camera.fov, 1.0f, 0.0f, 120.0f);
  ImGui::DragFloat("camera.near", &camera.near, 0.001f, 0.001f, 1.0f);
  ImGui::DragFloat("camera.far", &camera.far, 0.1f, 1.0f, 100.0f);

  ImGui::Text("%.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
  ImGui::End();
}

Game::Game(Platform *platform) : m_platform{platform} {
  m_mesh_groups.emplace_back(Import("assets/fullroom/fullroom.obj"));
  m_rp_material.emplace_back(m_mesh_groups[0].GetMaterials(),
                             m_mesh_groups[0].GetVertexBuffer(),
                             m_mesh_groups[0].GetElementBuffer());
  m_rp_depth_map.emplace_back(2048);
  m_rp_tex.emplace_back();
  m_rp_icon.emplace_back();
  m_rp_terrain.emplace_back();
  m_shader_material.emplace_back();
  m_light = {.ambient_color = {1.0f, 1.0f, 1.0f},
             .direction = {glm::normalize(glm::vec3(-0.2f, 1.0f, 0.2f))},
             .position = {-1.0f, 1.0f, 0.5f},
             .diffuse_color = {1.0f, 1.0f, 1.0f}};

  glm::vec3 initial_camera_position{2, 2, 2};
  glm::vec3 initial_camera_target{initial_camera_position +
                                  glm::vec3(0, 0, -1)};
  glm::mat4 transform{glm::lookAt(initial_camera_position, // position
                                  initial_camera_target,   // target
                                  glm::vec3{0, 1, 0})};    // up
  glm::vec2 drawable_size{m_platform->GetDrawableSize()};
  m_camera = {.transform = transform,
              .target = initial_camera_target,
              .aspect_ratio = 1.0f * drawable_size.x / drawable_size.y,
              .fov = 45,
              .near = 0.01f,
              .far = 64.0f};
  m_model_matrix =
      glm::rotate(glm::mat4(1.0f), -1.0f, glm::vec3(0.0, 1.0, 0.0));
  m_terrain_matrix = glm::mat4(1.0f);
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
    case SDLK_q:
      m_platform->QueueQuit();
      break;
    }
    break;
  }
  case SDL_MOUSEWHEEL: {
    MoveAlongCameraAxes(m_camera.transform,
                        glm::vec3(0, 0, 1.0f * event.motion.x * .01f));
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
  float AMOUNT = .01f;
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
    MoveAlongCameraAxes(camera.transform, movement * AMOUNT);
  }
  if (x != 0 || y != 0) {
    float SENSITIVITY = 0.005f;
    float amount_right = 1.0 * x * SENSITIVITY;
    float amount_up = -1.0 * y * SENSITIVITY;
    if (mouse_buttons & SDL_BUTTON_MMASK) {
      if (keyboard_buttons[SDL_SCANCODE_LSHIFT]) {
        SlideViewWithTarget(camera.transform, camera.target, amount_right,
                            amount_up);
      } else {
        if (amount_up != 0) {
          OrbitPitch(camera.transform, camera.target, amount_up);
        }
        if (amount_right != 0) {
          OrbitYaw(camera.transform, camera.target, -amount_right);
        }
      }
    }
    if (mouse_buttons & SDL_BUTTON_RMASK) {
      if (amount_up != 0) {
        RotatePitch(camera.transform, amount_up);
      }
      if (amount_right != 0) {
        RotateYaw(camera.transform, -amount_right);
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
  // glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, m_camera.near, m_camera.far);
  glm::mat4 vp = camera_projection * m_camera.transform;
  glm::mat4 model_vp = vp * m_model_matrix;
  glm::mat4 terrain_vp = vp * m_terrain_matrix;

  glm::mat4 light_transform =
      glm::lookAt(camera_position + glm::normalize(m_light.direction) * 5.0f,
                  camera_position, glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 light_projection = m_rp_depth_map[0].GetProjection();
  glm::mat4 light_vp = light_projection * light_transform;
  glm::mat4 model_light_vp = light_vp * m_model_matrix;
  glm::mat4 terrain_light_vp = light_vp * m_terrain_matrix;

  // shadow map pass
  m_rp_depth_map[0].Begin();
  m_rp_depth_map[0].SetMVP(model_light_vp);
  m_rp_material[0].DrawVertices();
  m_rp_depth_map[0].SetMVP(terrain_light_vp);
  m_rp_terrain[0].DrawVertices();
  m_rp_depth_map[0].End();

  // draw pass
  m_shader_material[0].Begin();

  m_shader_material[0].BindDepthTexture(m_rp_depth_map[0].GetFBO());

  m_shader_material[0].SetUniforms(camera_position, m_light, model_vp,
                                   model_light_vp, m_model_matrix);
  m_shader_material[0].BindMaterialsBuffer(
      m_rp_material[0].GetMaterialsBuffer());
  m_rp_material[0].DrawVertices();

  m_shader_material[0].SetUniforms(camera_position, m_light, terrain_vp,
                                   terrain_light_vp, m_terrain_matrix);
  m_shader_material[0].BindMaterialsBuffer(
      m_rp_terrain[0].GetMaterialsBuffer());
  m_rp_terrain[0].DrawVertices();

  m_shader_material[0].End();

  // draw shadow map to screen
  // m_rp_tex[0].draw(m_rp_depth_map[0].GetFBO());

  // 3d icons
  m_rp_icon[0].Draw(vp * glm::vec4(m_light.position, 1.0),
                    glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  m_rp_icon[0].Draw(vp * glm::vec4(m_camera.target, 1.0),
                    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  m_game_timer.t_finish_draw_calls = SDL_GetPerformanceCounter();
  RenderGui(m_game_timer, m_camera, m_light, m_model_matrix);
  m_game_timer.t_finish_gui_draw = SDL_GetPerformanceCounter();
  glFinish();
  m_game_timer.t_finish_render = SDL_GetPerformanceCounter();
}