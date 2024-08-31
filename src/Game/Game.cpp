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
  mRPMaterial.emplace(mMeshGroups[0].getMaterials(),
                      mMeshGroups[0].getVertexBuffer(),
                      mMeshGroups[0].getElementBuffer());
  mRPShadowMap.emplace(mRPMaterial.value().getVBO(),
                       mRPMaterial.value().getEBO(),
                       mMeshGroups[0].getNumElements(), 1024);
  mRPTex.emplace();
  mRPIcon.emplace();
  mRPTerrain.emplace();
  mLight = {.uAmbientLightColor = {.1f, .1f, .1f},
            .uLightDir = {-1.0f, 1.0f, 0.5f},
            .uLightPos = {-1.0f, 1.0f, 0.5f},
            .uLightColor = {1.0f, 1.0f, 1.0f}};

  glm::vec3 initialCameraTarget{0.0f, 0.5f, 0.0f};
  glm::mat4 transform{glm::lookAt(glm::vec3{0.0, 5.0, 5.0}, // position
                                  initialCameraTarget,      // target
                                  glm::vec3{0, 1, 0})};     // up
  glm::vec2 drawableSize{mPlatform->getDrawableSize()};
  mCamera = {.transform = transform,
             .target = initialCameraTarget,
             .aspectRatio = 1.0f * drawableSize.x / drawableSize.y,
             .fov = 45,
             .near = 0.01f,
             .far = 16.0f};
  muModelMatrix = glm::rotate(glm::mat4(1.0f), -1.0f, glm::vec3(0.0, 1.0, 0.0));
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

void Game::render() {
  mGameTimer.tFinishEvents = SDL_GetPerformanceCounter();
  glClear(GL_DEPTH_BUFFER_BIT);
  static const float bg[] = {0.2f, 0.2f, 0.2f, 1.0f};
  glClearBufferfv(GL_COLOR, 0, bg);

  glm::vec3 uCameraPos{getCameraPos(mCamera.transform)};
  glm::mat4 cameraProjection =
      glm::perspective(glm::radians(mCamera.fov), mCamera.aspectRatio,
                       mCamera.near, mCamera.far);
  glm::mat4 vp = cameraProjection * mCamera.transform;
  glm::mat4 uMVP = vp * muModelMatrix;

  glm::mat4 lightTransform =
      glm::lookAt(mLight.uLightPos, mCamera.target, glm::vec3(0.0, 1.0, 0.0));

  glm::mat4 lightProjection = mRPShadowMap.value().getProjection();
  glm::mat4 uLightMVP = lightProjection * lightTransform * muModelMatrix;

  // shadow map pass
  mRPShadowMap.value().draw(uLightMVP);

  // draw shadow map to screen
  // mRPTex.value().draw(mRPShadowMap.value().getFBO());

  // material + lighting pass
  mRPMaterial.value().draw(uCameraPos, mLight, uMVP, uLightMVP, muModelMatrix,
                           mRPShadowMap.value().getFBO());

  // terrain
  mRPTerrain.value().draw(uCameraPos, mLight, uMVP, uLightMVP, muModelMatrix,
                          mRPShadowMap.value().getFBO());
  // 3d icons
  mRPIcon.value().draw(vp * glm::vec4(mLight.uLightPos, 1.0),
                       glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
  mRPIcon.value().draw(vp * glm::vec4(mCamera.target, 1.0),
                       glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  mGameTimer.tFinishDrawCalls = SDL_GetPerformanceCounter();
  renderGUI(mGameTimer, mCamera, mLight, muModelMatrix);
  mGameTimer.tFinishGUIDraw = SDL_GetPerformanceCounter();
  glFinish();
  mGameTimer.tFinishRender = SDL_GetPerformanceCounter();
}
void Game::event(const SDL_Event &event) {
  switch (event.type) {
  case SDL_QUIT:
    mPlatform->queue_quit();
    break;
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_q:
      mPlatform->queue_quit();
      break;
    }
    break;
  case SDL_MOUSEWHEEL: {
    // zoom
    zoomCamera(mCamera.transform, mCamera.target, event.motion.x * 0.1f);
    break;
  }
  case SDL_MOUSEMOTION: {
    int x, y, l;
    Uint32 mouseButtons = SDL_GetMouseState(&x, &y);
    const Uint8 *keyboardButtons = SDL_GetKeyboardState(&l);
    if (mouseButtons & SDL_BUTTON_MMASK) {
      if (keyboardButtons[SDL_SCANCODE_LSHIFT]) {
        slideView(mCamera.transform, mCamera.target, event.motion.xrel,
                  event.motion.yrel);
      } else {
        if (event.motion.yrel != 0) {
          orbitPitch(mCamera.transform, mCamera.target, event.motion.yrel);
        }
        if (event.motion.xrel != 0) {
          orbitYaw(mCamera.transform, mCamera.target, event.motion.xrel);
        }
      }
    }
    break;
  }
  case SDL_WINDOWEVENT:
    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
      handle_resize(&event, mCamera);
    }
    break;
  }
}