#include <iostream>
#include <stdio.h>
#include <string>

#include "gl.hpp"

#include <imgui.h>
#define IMGUI_IMPL_OPENGL_ES3
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <SDL.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "MeshGroup.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "utils.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void GL_APIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                 GLenum severity, GLsizei length,
                                 const GLchar *message, const void *userParam) {
  if (type == GL_DEBUG_TYPE_OTHER &&
      severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }
  fprintf(stderr,
          "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
          message);
}

int SCREEN_WIDTH = 1600;
int SCREEN_HEIGHT = 1200;

static void handle_resize(SDL_Event *event, Camera &camera) {
  SCREEN_WIDTH = event->window.data1;
  SCREEN_HEIGHT = event->window.data2;
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  camera.aspectRatio = 1.0f * SCREEN_WIDTH / SCREEN_HEIGHT;
}

SDL_Window *gWindow = NULL;
SDL_GLContext ctx = NULL;

static void quit_game(int code) {
  SDL_DestroyWindow(gWindow);
  SDL_Quit();
  exit(code);
}

void initGLWindow(int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "SDL could not SDL_Init! SDL Error: " << SDL_GetError()
              << std::endl;
    quit_game(1);
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  gWindow = SDL_CreateWindow(
      "Blade3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
          SDL_WINDOW_BORDERLESS);
  if (gWindow == NULL) {
    std::cout << "Window could not be created! SDL Error: " << SDL_GetError()
              << std::endl;
    quit_game(1);
  }
  ctx = SDL_GL_CreateContext(gWindow);
  if (SDL_GL_SetSwapInterval(1) < 0) { // enable vsync
    std::cout << "SDL could not SetSwapInterval: " << SDL_GetError()
              << std::endl;
  }
  if (ctx == NULL) {
    std::cout << "Failed to create ctx." << std::endl;
    quit_game(1);
  }
}

void printGLInfo() {
  std::cout << "OpenGL loaded\n" << std::endl;
  std::cout << "Vendor:" << glGetString(GL_VENDOR) << std::endl;
  std::cout << "Renderer:" << glGetString(GL_RENDERER) << std::endl;
  std::cout << "Version:" << glGetString(GL_VERSION) << std::endl;
  std::cout << "Shading Language Version:"
            << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

MeshGroup import(const std::string &pFile) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(pFile, 0);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return nullptr;
  }
  return MeshGroup{scene};
}

void initImGui() {
  static const char *_IM_glsl_version = "#version 300 es";
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(gWindow, ctx);
  ImGui_ImplOpenGL3_Init(_IM_glsl_version);
}

void renderGUI(Uint64 cpu_us, Uint64 gui_us, Uint64 gpu_us, Camera &camera,
               Light &light, glm::mat4 &uModelMatrix) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  ImGuiIO &io = ImGui::GetIO();
  ImGui::Begin("Performance Counters");
  ImGui::Text("cpu=%luus", cpu_us);
  ImGui::Text("gui=%luus", gui_us);
  ImGui::Text("gpu=%luus", gpu_us);
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

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main(int argc, char *args[]) {
  initGLWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);
  printGLInfo();
  initImGui();

  MeshGroup meshGroup = import("assets/fullroom/fullroom.obj");
  RP_Material rpMaterial{meshGroup.getMaterials(), meshGroup.getVertexBuffer(),
                         meshGroup.getElementBuffer()};
  RP_ShadowMap rpShadowMap{rpMaterial.getVBO(), rpMaterial.getEBO(),
                           meshGroup.getNumElements(), 1024};
  RP_Tex rpTex{};
  RP_Icon rpIcon{};

  Light light{.uAmbientLightColor = {.1f, .1f, .1f},
              .uLightDir = {-1.0f, 1.0f, 0.5f},
              .uLightPos = {-1.0f, 1.0f, 0.5f},
              .uLightColor = {1.0f, 1.0f, 1.0f}};
  glm::vec3 initialCameraTarget{0.0f, 0.5f, 0.0f};
  glm::mat4 transform{glm::lookAt(glm::vec3{0.0, 5.0, 5.0}, // position
                                  initialCameraTarget,      // target
                                  glm::vec3{0, 1, 0})};     // up
  Camera camera{.transform = transform,
                .target = initialCameraTarget,
                .aspectRatio = 1.0f * SCREEN_WIDTH / SCREEN_HEIGHT,
                .fov = 45,
                .near = 0.01f,
                .far = 16.0f};
  glm::mat4 uModelMatrix{
      glm::rotate(glm::mat4(1.0f), -1.0f, glm::vec3(0.0, 1.0, 0.0))};

  SDL_Event event;
  bool quit = false;
  Uint64 tFrameStart{0};
  Uint64 tFinishDrawCalls{0};
  Uint64 tFinishGUIDraw{0};
  Uint64 tFinishRender{0};
  Uint64 countPerMicrosecond = SDL_GetPerformanceFrequency() / 1'000'000;
  while (quit == false) {
    Uint64 cpu_us = (tFinishDrawCalls - tFrameStart) / (countPerMicrosecond);
    Uint64 gui_us = (tFinishGUIDraw - tFinishDrawCalls) / (countPerMicrosecond);
    Uint64 gpu_us = (tFinishRender - tFinishGUIDraw) / (countPerMicrosecond);
    tFrameStart = SDL_GetPerformanceCounter();
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type) {
      case SDL_QUIT:
        quit = true;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_q:
          quit = true;
          break;
        }
        break;
      case SDL_MOUSEWHEEL: {
        // zoom
        zoomCamera(camera.transform, camera.target, event.motion.x * 0.1f);
        break;
      }
      case SDL_MOUSEMOTION: {
        int x, y, l;
        Uint32 mouseButtons = SDL_GetMouseState(&x, &y);
        const Uint8 *keyboardButtons = SDL_GetKeyboardState(&l);
        if (mouseButtons & SDL_BUTTON_MMASK) {
          if (keyboardButtons[SDL_SCANCODE_LSHIFT]) {
            slideView(camera.transform, camera.target, event.motion.xrel,
                      event.motion.yrel);
          } else {
            if (event.motion.yrel != 0) {
              orbitPitch(camera.transform, camera.target, event.motion.yrel);
            }
            if (event.motion.xrel != 0) {
              orbitYaw(camera.transform, camera.target, event.motion.xrel);
            }
          }
        }
        break;
      }
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          handle_resize(&event, camera);
        }
        break;
      }
    }
    glClear(GL_DEPTH_BUFFER_BIT);
    static const float bg[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, bg);

    glm::vec3 uCameraPos{getCameraPos(camera.transform)};
    glm::mat4 cameraProjection = glm::perspective(
        glm::radians(camera.fov), camera.aspectRatio, camera.near, camera.far);
    glm::mat4 vp = cameraProjection * camera.transform;
    glm::mat4 uMVP = vp * uModelMatrix;

    glm::mat4 lightTransform =
        glm::lookAt(light.uLightPos, camera.target, glm::vec3(0.0, 1.0, 0.0));

    glm::mat4 lightProjection = rpShadowMap.getProjection();
    glm::mat4 uLightMVP = lightProjection * lightTransform * uModelMatrix;

    // shadow map pass
    rpShadowMap.draw(uLightMVP);

    // draw shadow map to screen
    // rpTex.draw(rpShadowMap.getFBO());

    // material + lighting pass
    rpMaterial.draw(uCameraPos, light, uMVP, uLightMVP, uModelMatrix,
                    rpShadowMap.getFBO());

    // 3d icons
    rpIcon.draw(vp * glm::vec4(light.uLightPos, 1.0),
                glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    rpIcon.draw(vp * glm::vec4(camera.target, 1.0),
                glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    tFinishDrawCalls = SDL_GetPerformanceCounter();
    renderGUI(cpu_us, gui_us, gpu_us, camera, light, uModelMatrix);
    tFinishGUIDraw = SDL_GetPerformanceCounter();
    glFinish(); // block so we get an accurate frametime
    tFinishRender = SDL_GetPerformanceCounter();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(gWindow);
  }

  quit_game(0);
}