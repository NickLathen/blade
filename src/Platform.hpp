#pragma once

#include <SDL.h>
#include <glm/glm.hpp>

#include "Game.hpp"

class Platform {
public:
  Platform(int width, int height);
  ~Platform();
  static void queue_quit();
  glm::vec2 getDrawableSize();

  void loop(Game &game);

private:
  SDL_Window *mWindow = nullptr;
  SDL_GLContext mContext = nullptr;

  void shutdown();
  void beginImguiFrame();
  void endImguiFrame();
  void initGL(int width, int height);
  void initGLFunctions();
  void initImgui();
};