#include <SDL.h>
#include <imgui.h>
#define IMGUI_IMPL_OPENGL_ES3
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>

#include "Platform.hpp"
#include "gl.hpp"

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

void printGLInfo() {
  std::cout << "OpenGL loaded\n" << std::endl;
  std::cout << "Vendor:" << glGetString(GL_VENDOR) << std::endl;
  std::cout << "Renderer:" << glGetString(GL_RENDERER) << std::endl;
  std::cout << "Version:" << glGetString(GL_VERSION) << std::endl;
  std::cout << "Shading Language Version:"
            << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

Platform::Platform(int width, int height) {
  initGL(width, height);
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);
  printGLInfo();
  initImgui();
}
Platform::~Platform() { shutdown(); }

void Platform::queue_quit() {
  SDL_Event e;
  e.type = SDL_QUIT;
  SDL_PushEvent(&e);
}
glm::vec2 Platform::getDrawableSize() {
  int w, h;
  SDL_GL_GetDrawableSize(mWindow, &w, &h);
  return glm::vec2(w, h);
}
void Platform::loop(Game &game) {
  while (1) {
    game.beginFrame();
    beginImguiFrame();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      switch (e.type) {
      case SDL_QUIT:
        return; // end the loop
      }
      game.event(e);
    }
    game.render();
    endImguiFrame();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(mWindow);
  }
}
void Platform::shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(mContext);
  SDL_DestroyWindow(mWindow);
}
void Platform::beginImguiFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}
void Platform::endImguiFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Platform::initGL(int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "SDL could not SDL_Init! SDL Error: " << SDL_GetError()
              << std::endl;
    exit(1);
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
  mWindow = SDL_CreateWindow(
      "Blade3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
          SDL_WINDOW_BORDERLESS);
  if (mWindow == NULL) {
    std::cout << "Window could not be created! SDL Error: " << SDL_GetError()
              << std::endl;
    exit(1);
  }
  mContext = SDL_GL_CreateContext(mWindow);
  if (SDL_GL_SetSwapInterval(1) < 0) { // enable vsync
    std::cout << "SDL could not SetSwapInterval: " << SDL_GetError()
              << std::endl;
  }
  if (mContext == NULL) {
    std::cout << "Failed to create mContext." << std::endl;
    exit(1);
  }
}

void Platform::initImgui() {
  static const char *_IM_glsl_version = "#version 300 es";
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(mWindow, mContext);
  ImGui_ImplOpenGL3_Init(_IM_glsl_version);
}
