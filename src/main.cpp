#include "Game.hpp"
#include "Platform.hpp"

int main(int argc, char *args[]) {
  const int kScreenWidth = 1600;
  const int kScreenHeight = 1200;
  Platform platform{kScreenWidth, kScreenHeight};
  Game game{&platform};
  platform.Loop(game);
}