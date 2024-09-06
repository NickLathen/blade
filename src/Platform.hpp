#pragma once

#include <SDL.h>
#include <imgui.h>
#define IMGUI_IMPL_OPENGL_ES3
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <glm/glm.hpp>

#include "Game.hpp"

class Platform {
public:
  Platform(int width, int height);
  ~Platform();
  static void QueueQuit();
  glm::vec2 GetDrawableSize();

  void Loop(Game &game);

private:
  SDL_Window *m_window = nullptr;
  SDL_GLContext m_context = nullptr;

  void Shutdown();
  void BeginImguiFrame();
  void EndImguiFrame();
  void InitGL(int width, int height);
  void InitGLFunctions();
  void InitImgui();
};