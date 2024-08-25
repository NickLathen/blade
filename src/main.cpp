#include "gl.hpp"

#include <SDL2/SDL.h>
#include <iostream>
#include <stdio.h>
#include <string>

#include "Scene.hpp"
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
  throw;
}

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 1200;

SDL_Window *gWindow = NULL;
SDL_GLContext ctx = NULL;

static void quit_game(int code) {
  SDL_DestroyWindow(gWindow);
  SDL_Quit();
  exit(code);
}

void initGLWindow() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError()
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
      SDL_CreateWindow("SDL Game", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                           SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS);
  if (gWindow == NULL) {
    std::cout << "Window could not be created! SDL Error: " << SDL_GetError()
              << std::endl;
    quit_game(1);
  }
  ctx = SDL_GL_CreateContext(gWindow);
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

Scene import(const std::string &pFile) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(pFile, 0);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return nullptr;
  }
  return Scene{scene};
}

int main(int argc, char *args[]) {
  initGLWindow();
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);
  printGLInfo();
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glEnable(GL_DEPTH_TEST);

  Shader shader{"shaders/vertex.glsl", "shaders/fragment.glsl"};
  shader.setUniformBlockBinding("uMaterialBlock", 0);
  Scene s = import("assets/fullroom/fullroom.obj");

  SDL_Event e;
  bool quit = false;
  while (quit == false) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }
      if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_q:
          quit = true;
          break;
        }
      }
    }
    glClear(GL_DEPTH_BUFFER_BIT);
    static const float bg[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, bg);

    shader.useProgram();

    glm::vec3 uCameraPos{2.0f, 2.0f, 2.0f};
    glm::vec3 uAmbientLightColor = glm::vec3{.3, .3, .3};
    glm::vec3 uLightDir = glm::vec3(-1, -5, 1);
    glm::vec3 uLightColor = glm::vec3{1, 1, 1};

    glUniform3fv(shader.getUniformLocation("uCameraPos"), 1,
                 glm::value_ptr(uCameraPos));
    glUniform3fv(shader.getUniformLocation("uAmbientLightColor"), 1,
                 glm::value_ptr(uAmbientLightColor));
    glUniform3fv(shader.getUniformLocation("uLightDir"), 1,
                 glm::value_ptr(uLightDir));
    glUniform3fv(shader.getUniformLocation("uLightColor"), 1,
                 glm::value_ptr(uLightColor));

    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 cameraTargetPos{0.0f, 0.5f, 0.0f};
    glm::mat4 view = glm::lookAt(uCameraPos, cameraTargetPos, up);
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);
    glm::mat4 vp = projection * view;
    // rotate model
    float initial = 0;
    float end = M_PIf * 2.0;
    float rotation = (float)SDL_GetTicks64() / 2000 + initial;
    if (rotation > end) {
      rotation = fmodf(rotation, (initial - end)) + initial;
    }
    glm::mat4 model{1.0};
    glm::vec3 yAxis{0, 1, 0};
    model = glm::rotate(model, rotation, yAxis);

    s.draw(shader, vp, model, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(gWindow);
  }

  quit_game(0);
}