SANITIZE=-fsanitize=undefined
WARNALL=-Wall
DEBUG=-g
STD=--std=c++17
LDLIBS=-lstdc++ -lubsan -lSDL2 -lGLESv2 -lassimp
INCLUDES=-I./include -I./imgui -I./imgui/backends -I/usr/include/SDL2
CXX=clang
CXXFLAGS=$(STD) $(SANITIZE) $(WARNALL) $(DEBUG) $(INCLUDES)

BUILD_FILES=src/main.cpp \
            src/Platform.cpp \
            src/utils.cpp \
            src/Shader.cpp \
            src/MeshGroup.cpp \
            src/Material.cpp \
            src/Mesh.cpp \
            src/RP_DepthMap.cpp \
            src/RP_Icon.cpp \
            src/RP_Tex.cpp \
            src/RP_Material.cpp \
            src/RP_Terrain.cpp

GAME_FILES=src/Game/Game.cpp

IMGUI_BUILD_FILES=imgui/imgui.cpp \
                  imgui/backends/imgui_impl_sdl2.cpp \
                  imgui/backends/imgui_impl_opengl3.cpp \
                  imgui/imgui_draw.cpp \
                  imgui/imgui_tables.cpp \
                  imgui/imgui_widgets.cpp \
                  imgui/imgui_demo.cpp
SOURCES=$(BUILD_FILES) $(GAME_FILES) $(IMGUI_BUILD_FILES)
OBJS=$(addprefix build/, $(addsuffix .o, $(basename $(SOURCES))))

all: build/main

build/main: $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDLIBS) -o $@ $^

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/imgui/%.o: imgui/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/imgui/backends/%.o: imgui/backends/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build/src

clean_all:
	rm -rf build/

.PHONY: all clean