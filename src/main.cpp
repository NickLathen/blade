#include "Game.hpp"
#include "Platform.hpp"

int main(int argc, char *args[]) {
  const int kScreenWidth = 2000;
  const int kScreenHeight = 1500;
  Platform platform{kScreenWidth, kScreenHeight};
  Game game{&platform};
  platform.Loop(game);
}