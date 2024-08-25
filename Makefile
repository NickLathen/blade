SANITIZE=-fsanitize=undefined
WARNALL=-Wall
DEBUG=-g
STD=--std=c++17
LDLIBS=-lstdc++ -lubsan -lSDL2 -lGLESv2 -lassimp
INCLUDES=-I./include
CXX=clang
CXXFLAGS=$(STD) $(SANITIZE) $(WARNALL) $(DEBUG) $(INCLUDES)

SOURCES=src/main.cpp \
        src/utils.cpp \
		src/Shader.cpp \
		src/Actor.cpp \
		src/Material.cpp \
		src/Mesh.cpp \
		src/Entity.cpp

OBJS=$(addprefix build/, $(addsuffix .o, $(basename $(SOURCES))))

all: build/main

build/main: $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDLIBS) -o $@ $^

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build

.PHONY: all clean