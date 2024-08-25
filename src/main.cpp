#include <iostream>
#include <stdio.h>
#include <string>

#include "gl.hpp"

#include <imgui.h>
#define IMGUI_IMPL_OPENGL_ES3
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <SDL.h>

#include "Actor.hpp"
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

static void handle_resize(SDL_Event *event) {
  SCREEN_WIDTH = event->window.data1;
  SCREEN_HEIGHT = event->window.data2;
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

SDL_Window *gWindow = NULL;
SDL_GLContext ctx = NULL;

static void quit_game(int code) {
  SDL_DestroyWindow(gWindow);
  SDL_Quit();
  exit(code);
}

void initGLWindow() {
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
  gWindow =
      SDL_CreateWindow("Blade3D", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                           SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS);
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

Actor import(const std::string &pFile) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(pFile, 0);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return nullptr;
  }
  return Actor{scene};
}

void initImGui() {
  static const char *_IM_glsl_version = "#version 300 es";
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(gWindow, ctx);
  ImGui_ImplOpenGL3_Init(_IM_glsl_version);
}

void renderGUI(Uint64 frametime_us) {
  ImGuiIO &io = ImGui::GetIO();
  // Render ImGui
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Performance Counters");
  ImGui::Text("frametime=%luus", frametime_us);
  ImGui::Text("%.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main(int argc, char *args[]) {
  initGLWindow();
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);
  printGLInfo();
  initImGui();

  Actor s = import("assets/fullroom/fullroom.obj");

  SDL_Event event;
  bool quit = false;
  Uint64 tFrameStart{0};
  Uint64 tFinishDrawCalls{0};
  Uint64 countPerMicrosecond = SDL_GetPerformanceFrequency() / 1'000'000;
  while (quit == false) {
    Uint64 frametime_us =
        (tFinishDrawCalls - tFrameStart) / (countPerMicrosecond);
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
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          handle_resize(&event);
        }
        break;
      }
    }
    glClear(GL_DEPTH_BUFFER_BIT);
    static const float bg[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, bg);

    // rotate actor
    float initial = 0;
    float end = M_PIf * 2.0;
    float rotation = (float)SDL_GetTicks64() / 2000 + initial;
    if (rotation > end) {
      rotation = fmodf(rotation, (initial - end)) + initial;
    }
    glm::mat4 actorTransform{1.0};
    glm::vec3 yAxis{0, 1, 0};
    actorTransform = glm::rotate(actorTransform, rotation, yAxis);

    glm::vec3 cameraPos{2.0, 2.0, 2.0};
    float aspectRatio = 1.0 * SCREEN_WIDTH / SCREEN_HEIGHT;

    s.draw(cameraPos, aspectRatio, actorTransform);
    renderGUI(frametime_us);
    tFinishDrawCalls = SDL_GetPerformanceCounter();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(gWindow);
    glFinish(); // block on window swap so we get an accurate frametime
  }

  quit_game(0);
}