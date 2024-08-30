#include "Game.hpp"
#include "Platform.hpp"

int SCREEN_WIDTH = 1600;
int SCREEN_HEIGHT = 1200;

int main(int argc, char *args[]) {
  Platform platform{SCREEN_WIDTH, SCREEN_HEIGHT};
  Game game{&platform};
  platform.loop(game);
}