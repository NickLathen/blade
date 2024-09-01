#include <imgui.h>
#include <iostream>

#include "../Game.hpp"
#include "../MeshGroup.hpp"
#include "../Platform.hpp"
#include "../RenderPass.hpp"
#include "../utils.hpp"

static void handle_resize(const SDL_Event *event, Camera &camera) {
  int x = event->window.data1;
  int y = event->window.data2;
  glViewport(0, 0, x, y);
  camera.aspectRatio = 1.0f * x / y;
}

void renderGUI(const GameTimer &gameTimer, Camera &camera, Light &light,
               glm::mat4 &uModelMatrix) {

  ImGuiIO &io = ImGui::GetIO();
  ImGui::Begin("Performance Counters");
  ImGui::Text("evt=%luus", gameTimer.evt_us);
  ImGui::Text("cpu=%luus", gameTimer.cpu_us);
  ImGui::Text("gui=%luus", gameTimer.gui_us);
  ImGui::Text("gpu=%luus", gameTimer.gpu_us);
  ImGui::DragFloat4("camera.transform[0]", &camera.transform[0][0], .01f, -5.0f,
                    5.0f);
  ImGui::DragFloat4("camera.transform[1]", &camera.transform[1][0], .01f, -5.0f,
                    5.0f);
  ImGui::DragFloat4("camera.transform[2]", &camera.transform[2][0], .01f, -5.0f,
                    5.0f);
  ImGui::DragFloat4("camera.transform[3]", &camera.transform[3][0], .01f, -5.0f,
                    5.0f);
  ImGui::DragFloat4("uModelMatrix[0]", &uModelMatrix[0][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat4("uModelMatrix[1]", &uModelMatrix[1][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat4("uModelMatrix[2]", &uModelMatrix[2][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat4("uModelMatrix[3]", &uModelMatrix[3][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat3("camera.target", &camera.target[0], .01f, -10.0, 10.0f);
  ImGui::DragFloat3("light.uLightPos", &light.uLightPos[0], .01f, -10.0f,
                    10.0f);
  ImGui::DragFloat("camera.fov", &camera.fov, 1.0f, 0.0f, 120.0f);
  ImGui::DragFloat("camera.near", &camera.near, 0.001f, 0.001f, 1.0f);
  ImGui::DragFloat("camera.far", &camera.far, 0.1f, 1.0f, 100.0f);

  ImGui::Text("%.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
  ImGui::End();
}

Game::Game(Platform *platform) : mPlatform{platform} {
  mMeshGroups.emplace_back(import("assets/fullroom/fullroom.obj"));
  mRPMaterial.emplace_back(mMeshGroups[0].getMaterials(),
                           mMeshGroups[0].getVertexBuffer(),
                           mMeshGroups[0].getElementBuffer());
  mRPDepthMap.emplace_back(1024);
  mRPTex.emplace_back();
  mRPIcon.emplace_back();
  mRPTerrain.emplace_back();
  mLight = {.uAmbientLightColor = {.2f, .2f, .2f},
            .uLightDir = {-1.0f, 1.0f, 0.5f},
            .uLightPos = {-1.0f, 1.0f, 0.5f},
            .uLightColor = {1.0f, 1.0f, 1.0f}};

  glm::vec3 initialCameraPos{2, 2, 2};
  glm::vec3 initialCameraTarget{initialCameraPos + glm::vec3(0, 0, -1)};
  glm::mat4 transform{glm::lookAt(initialCameraPos,     // position
                                  initialCameraTarget,  // target
                                  glm::vec3{0, 1, 0})}; // up
  glm::vec2 drawableSize{mPlatform->getDrawableSize()};
  mCamera = {.transform = transform,
             .target = initialCameraTarget,
             .aspectRatio = 1.0f * drawableSize.x / drawableSize.y,
             .fov = 45,
             .near = 0.01f,
             .far = 64.0f};
  muModelMatrix = glm::rotate(glm::mat4(1.0f), -1.0f, glm::vec3(0.0, 1.0, 0.0));
  muTerrainMatrix = glm::mat4(1.0f);
  mGameTimer.countPerMicrosecond = SDL_GetPerformanceFrequency() / 1'000'000;
}
void Game::beginFrame() {
  mGameTimer.evt_us = (mGameTimer.tFinishEvents - mGameTimer.tFrameStart) /
                      (mGameTimer.countPerMicrosecond);
  mGameTimer.cpu_us = (mGameTimer.tFinishDrawCalls - mGameTimer.tFinishEvents) /
                      (mGameTimer.countPerMicrosecond);
  mGameTimer.gui_us =
      (mGameTimer.tFinishGUIDraw - mGameTimer.tFinishDrawCalls) /
      (mGameTimer.countPerMicrosecond);
  mGameTimer.gpu_us = (mGameTimer.tFinishRender - mGameTimer.tFinishGUIDraw) /
                      (mGameTimer.countPerMicrosecond);
  mGameTimer.tFrameStart = SDL_GetPerformanceCounter();
}
void Game::event(const SDL_Event &event) {
  switch (event.type) {
  case SDL_QUIT:
    mPlatform->queue_quit();
    break;
  case SDL_KEYDOWN: {
    switch (event.key.keysym.sym) {
    case SDLK_q:
      mPlatform->queue_quit();
      break;
    }
    break;
  }
  case SDL_MOUSEWHEEL: {
    moveAlongCameraAxes(mCamera.transform,
                        glm::vec3(0, 0, 1.0f * event.motion.x * .01f));
    break;
  }
  case SDL_WINDOWEVENT:
    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
      handle_resize(&event, mCamera);
    }
    break;
  }
}
void handleInput(Camera &camera) {
  float AMOUNT = .01f;
  int x, y, l;
  Uint32 mouseButtons = SDL_GetRelativeMouseState(&x, &y);
  const Uint8 *keyboardButtons = SDL_GetKeyboardState(&l);
  glm::vec3 movement{0.0};
  if (keyboardButtons[SDL_SCANCODE_W]) {
    movement.z += 1;
  }
  if (keyboardButtons[SDL_SCANCODE_A]) {
    movement.x -= 1;
  }
  if (keyboardButtons[SDL_SCANCODE_S]) {
    movement.z -= 1;
  }
  if (keyboardButtons[SDL_SCANCODE_D]) {
    movement.x += 1;
  }
  if (keyboardButtons[SDL_SCANCODE_R]) {
    movement.y += 1;
  }
  if (keyboardButtons[SDL_SCANCODE_F]) {
    movement.y -= 1;
  }
  if (movement != glm::vec3{0.0}) {
    movement = glm::normalize(movement);
    moveAlongCameraAxes(camera.transform, movement * AMOUNT);
  }
  if (x != 0 || y != 0) {
    float SENSITIVITY = 0.005f;
    float xRel = 1.0 * x * SENSITIVITY;
    float yRel = -1.0 * y * SENSITIVITY;
    if (mouseButtons & SDL_BUTTON_MMASK) {
      if (keyboardButtons[SDL_SCANCODE_LSHIFT]) {
        slideView(camera.transform, camera.target, xRel, yRel);
      } else {
        if (yRel != 0) {
          orbitPitch(camera.transform, camera.target, yRel);
        }
        if (xRel != 0) {
          orbitYaw(camera.transform, camera.target, -xRel);
        }
      }
    }
    if (mouseButtons & SDL_BUTTON_RMASK) {
      if (yRel != 0) {
        rotatePitch(camera.transform, yRel);
      }
      if (xRel != 0) {
        rotateYaw(camera.transform, -xRel);
      }
    }
  }
}
void Game::render() {
  handleInput(mCamera);
  mGameTimer.tFinishEvents = SDL_GetPerformanceCounter();
  glClear(GL_DEPTH_BUFFER_BIT);
  static const float bg[] = {0.2f, 0.2f, 0.2f, 1.0f};
  glClearBufferfv(GL_COLOR, 0, bg);

  glm::vec3 uCameraPos{getCameraPos(mCamera.transform)};
  glm::mat4 cameraProjection =
      glm::perspective(glm::radians(mCamera.fov), mCamera.aspectRatio,
                       mCamera.near, mCamera.far);
  // glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, mCamera.near, mCamera.far);
  glm::mat4 vp = cameraProjection * mCamera.transform;
  glm::mat4 uModelVP = vp * muModelMatrix;
  glm::mat4 uTerrainVP = vp * muTerrainMatrix;

  glm::mat4 lightTransform =
      glm::lookAt(mLight.uLightPos, mCamera.target, glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 lightProjection = mRPDepthMap[0].getProjection();
  glm::mat4 lightVP = lightProjection * lightTransform;
  glm::mat4 uModelLightVP = lightVP * muModelMatrix;
  glm::mat4 uTerrainLightVP = lightVP * muTerrainMatrix;

  // shadow map pass
  mRPDepthMap[0].begin();
  mRPDepthMap[0].setMVP(uModelLightVP);
  mRPMaterial[0].drawVertices();
  mRPDepthMap[0].setMVP(uTerrainLightVP);
  mRPTerrain[0].drawVertices();
  mRPDepthMap[0].end();

  // draw pass
  mRPMaterial[0].draw(uCameraPos, mLight, uModelVP, uModelLightVP,
                      muModelMatrix, mRPDepthMap[0].getFBO());
  mRPTerrain[0].draw(uCameraPos, mLight, uTerrainVP, uTerrainLightVP,
                     muTerrainMatrix, mRPDepthMap[0].getFBO());

  // draw shadow map to screen
  // mRPTex[0].draw(mRPDepthMap[0].getFBO());

  // 3d icons
  mRPIcon[0].draw(vp * glm::vec4(mLight.uLightPos, 1.0),
                  glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  mRPIcon[0].draw(vp * glm::vec4(mCamera.target, 1.0),
                  glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  mGameTimer.tFinishDrawCalls = SDL_GetPerformanceCounter();
  renderGUI(mGameTimer, mCamera, mLight, muModelMatrix);
  mGameTimer.tFinishGUIDraw = SDL_GetPerformanceCounter();
  glFinish();
  mGameTimer.tFinishRender = SDL_GetPerformanceCounter();
}