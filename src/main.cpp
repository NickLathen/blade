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
               Light &light) {
  ImGuiIO &io = ImGui::GetIO();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Performance Counters");
  ImGui::Text("cpu=%luus", cpu_us);
  ImGui::Text("gui=%luus", gui_us);
  ImGui::Text("gpu=%luus", gpu_us);
  ImGui::DragFloat4("matrix0", &camera.transform[0][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat4("matrix1", &camera.transform[1][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat4("matrix2", &camera.transform[2][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat4("matrix3", &camera.transform[3][0], .01f, -5.0f, 5.0f);
  ImGui::DragFloat3("light.uLightDir", &light.uLightDir[0], .01f, -10.0, 10.0f);
  ImGui::DragFloat3("light.uLightPos", &light.uLightPos[0], .01f, -10.0f,
                    10.0f);
  ImGui::DragFloat("camera.fov", &camera.fov, 1.0f, 0.0f, 120.0f);
  ImGui::DragFloat("camera.near", &camera.near, 0.1f, 0.1f, 10.0f);
  ImGui::DragFloat("camera.far", &camera.far, 0.5f, 1.0f, 100.0f);

  ImGui::Text("%.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void zoomCamera(glm::mat4 &viewMatrix, glm::vec3 &target, float zoomAmount) {
  glm::vec3 cameraForward =
      glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
  cameraForward = glm::normalize(cameraForward);
  // prevent zooming through target

  viewMatrix = glm::translate(viewMatrix, cameraForward * zoomAmount);
}
void orbitYaw(glm::mat4 &viewMatrix, glm::vec3 &target, float amount) {
  glm::mat4 translated = glm::translate(viewMatrix, target);
  glm::mat4 rotated =
      glm::rotate(translated, amount * 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
  viewMatrix = glm::translate(rotated, -target);
};
void orbitPitch(glm::mat4 &viewMatrix, glm::vec3 &target, float amount) {
  glm::mat4 translated = glm::translate(viewMatrix, target);
  glm::vec3 cameraRight =
      glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
  cameraRight = glm::normalize(cameraRight);
  glm::mat4 rotated = glm::rotate(translated, amount * 0.01f, cameraRight);
  viewMatrix = glm::translate(rotated, -target);
};

void slideView(glm::mat4 &viewMatrix, glm::vec3 &target, float xAmount,
               float yAmount) {
  glm::vec3 cameraRight =
      glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
  cameraRight = glm::normalize(cameraRight);
  glm::vec3 cameraUp =
      glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
  cameraUp = glm::normalize(cameraUp);
  glm::vec3 translation =
      cameraUp * yAmount * 0.01f + cameraRight * -xAmount * 0.01f;
  viewMatrix = glm::translate(viewMatrix, translation);
  target -= translation;
};

void initIconShader(const Shader &iconShader, GLuint &iconVAO) {
  iconShader.useProgram();
  glGenVertexArrays(1, &iconVAO);
  glBindVertexArray(iconVAO);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}
void drawIcon(const Shader &iconShader, GLuint iconVAO, glm::vec4 position,
              glm::vec4 color) {
  iconShader.useProgram();
  glBindVertexArray(iconVAO);
  glUniform4fv(iconShader.getUniformLocation("uPos"), 1,
               glm::value_ptr(position));
  glUniform1ui(iconShader.getUniformLocation("uColor"),
               glm::packUnorm4x8(color));
  glDrawArrays(GL_POINTS, 0, 1);
  glBindVertexArray(0);
}

int main(int argc, char *args[]) {
  initGLWindow();
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);
  printGLInfo();
  initImGui();

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
                .near = 0.1f,
                .far = 16.0f};

  Actor s = import("assets/fullroom/fullroom.obj");

  Shader iconShader{"shaders/icon_vertex.glsl", "shaders/icon_fragment.glsl"};
  GLuint iconVAO;
  initIconShader(iconShader, iconVAO);

  Shader shadowShader{"shaders/shadow_map_vertex.glsl",
                      "shaders/shadow_map_fragment.glsl"};
  GLuint shadowVAO, FBO, shadowMapTexture;
  glGenFramebuffers(1, &FBO);
  glGenTextures(1, &shadowMapTexture);
  glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SCREEN_WIDTH,
               SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                         GL_TEXTURE_2D, shadowMapTexture, 0);

  GLenum Status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

  if (Status != GL_FRAMEBUFFER_COMPLETE) {
    printf("FB error, status: 0x%x\n", Status);
    return 1;
  }
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  Shader texShader("shaders/texture_vertex.glsl",
                   "shaders/texture_fragment.glsl");
  GLuint texVAO, texVBO;
  glGenVertexArrays(1, &texVAO);
  glGenBuffers(1, &texVBO);
  glBindVertexArray(texVAO);
  glBindBuffer(GL_ARRAY_BUFFER, texVBO);
  struct TexVert {
    glm::vec3 aPos;      // -1 to 1
    glm::vec2 aTexCoord; // 0 to 1
  };
  TexVert texVerts[3]{
      {glm::vec3(-1.0, 3.0, 0.0f), glm::vec2(0.0, 2.0)},  // upper left
      {glm::vec3(-1.0, -1.0, 0.0f), glm::vec2(0.0, 0.0)}, // bottom left
      {glm::vec3(3.0, -1.0, 0.0f), glm::vec2(2.0, 0.0)}}; // bottom right
  glBufferData(GL_ARRAY_BUFFER, sizeof(texVerts), &texVerts[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5,
                        (void *)(sizeof(float) * 3));
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  shadowShader.useProgram();
  glGenVertexArrays(1, &shadowVAO);
  glBindVertexArray(shadowVAO);
  glBindBuffer(GL_ARRAY_BUFFER, s.mVBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexBuffer),
                        (GLvoid *)0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s.mEBO);
  glBindVertexArray(0);
  glUseProgram(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

    glm::mat4 projection = glm::perspective(
        glm::radians(camera.fov), camera.aspectRatio, camera.near, camera.far);
    glm::mat4 mvp = projection * camera.transform;

    // shadow map pass
    glBindVertexArray(shadowVAO);
    shadowShader.useProgram();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glm::mat4 lightTransform =
        glm::lookAt(light.uLightPos, camera.target, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightProjection = glm::perspective(
        glm::radians(90.0f), camera.aspectRatio, camera.near, camera.far);
    glm::mat4 uLightMVP = lightProjection * lightTransform;

    glUniformMatrix4fv(shadowShader.getUniformLocation("uMVP"), 1, GL_FALSE,
                       glm::value_ptr(uLightMVP));
    glDrawElements(GL_TRIANGLES, s.getNumElements(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // render shadowmap
    // glBindVertexArray(texVAO);
    // texShader.useProgram();
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glUniform1i(texShader.getUniformLocation("uTexture"), 0);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, FBO);
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    // glBindVertexArray(0);
    // glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    s.draw(camera, light, FBO);

    glm::vec4 lightPosition = mvp * glm::vec4(light.uLightPos, 1.0);
    glm::vec4 targetPosition = mvp * glm::vec4(camera.target, 1.0);
    drawIcon(iconShader, iconVAO, lightPosition,
             glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    drawIcon(iconShader, iconVAO, targetPosition,
             glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    tFinishDrawCalls = SDL_GetPerformanceCounter();
    renderGUI(cpu_us, gui_us, gpu_us, camera, light);
    tFinishGUIDraw = SDL_GetPerformanceCounter();
    glFinish(); // block so we get an accurate frametime
    tFinishRender = SDL_GetPerformanceCounter();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(gWindow);
  }

  quit_game(0);
}