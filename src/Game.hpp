#pragma once

#include "MeshGroup.hpp"
#include "RenderPass.hpp"
#include <SDL.h>
#include <optional>

struct GameTimer {
  Uint64 tFrameStart{0};
  Uint64 tFinishEvents{0};
  Uint64 tFinishDrawCalls{0};
  Uint64 tFinishGUIDraw{0};
  Uint64 tFinishRender{0};
  Uint64 countPerMicrosecond{0};
  Uint64 evt_us{0};
  Uint64 cpu_us{0};
  Uint64 gui_us{0};
  Uint64 gpu_us{0};
};

class Platform;
class Game {
public:
  Game(Platform *platform);
  void beginFrame();
  void render();
  void event(const SDL_Event &event);

private:
  Platform *mPlatform;
  Light mLight;
  Camera mCamera;
  glm::mat4 muModelMatrix;
  std::vector<MeshGroup> mMeshGroups{};
  std::optional<RP_Material> mRPMaterial{};
  std::optional<RP_ShadowMap> mRPShadowMap{};
  std::optional<RP_Tex> mRPTex{};
  std::optional<RP_Icon> mRPIcon{};
  std::optional<RP_Terrain> mRPTerrain{};
  GameTimer mGameTimer{};
};