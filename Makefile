# ORI - Procedural Origami Folding Studio
# Build with:  make          (app + studio)  make test    (unit tests)
#              make run      (render every demo into output/)
#              make studio   (the interactive studio, needs SDL2)
#              make lib      (library modules only)
#              make packages (which member packages are in the tree)
#              make clean
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

LIB_SRC    := $(wildcard src/*/*.cpp)
DEMO_SRC   := $(wildcard demos/*.cpp)
STUDIO_SRC := $(wildcard studio/*.cpp)
TEST_SRC   := $(wildcard tests/*.cpp)
LIB_OBJ    := $(patsubst %.cpp,build/%.o,$(LIB_SRC))
DEMO_OBJ   := $(patsubst %.cpp,build/%.o,$(DEMO_SRC))
STUDIO_OBJ := $(patsubst %.cpp,build/%.o,$(STUDIO_SRC))
TEST_OBJ   := $(patsubst %.cpp,build/%.o,$(TEST_SRC))

# SDL2 only opens the studio window; everything is drawn by our rasterizer.
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null || sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null || sdl2-config --libs 2>/dev/null)

ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

APP    := build/ori$(EXE)
STUDIO := build/ori_studio$(EXE)
TEST   := build/ori_tests$(EXE)

.PHONY: all lib test run studio run-studio packages clean

all: $(APP) $(if $(SDL_LIBS),$(STUDIO))

# Library only - useful while packages are still being pushed.
lib: $(LIB_OBJ)

$(APP): $(LIB_OBJ) $(DEMO_OBJ) build/app/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(STUDIO): $(LIB_OBJ) $(DEMO_OBJ) $(STUDIO_OBJ) build/studio/sdl/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(SDL_LIBS)

$(TEST): $(LIB_OBJ) $(DEMO_OBJ) $(STUDIO_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/studio/sdl/main.o: studio/sdl/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -MMD -MP -c $< -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

test: $(TEST)
	./$(TEST)

run: $(APP)
	./$(APP)

studio: $(STUDIO)

run-studio: $(STUDIO)
	./$(STUDIO)

packages:
	@sh scripts/ci_check.sh status

clean:
	rm -rf build output

-include $(LIB_OBJ:.o=.d) $(DEMO_OBJ:.o=.d) $(STUDIO_OBJ:.o=.d) $(TEST_OBJ:.o=.d) build/app/main.d build/studio/sdl/main.d
