# ORI - Procedural Origami Folding Studio
# Build with:  make          (app)      make test    (unit tests)
#              make run      (render every demo into output/)
#              make lib      (library modules only)
#              make clean
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

LIB_SRC  := $(wildcard src/*/*.cpp)
DEMO_SRC := $(wildcard demos/*.cpp)
TEST_SRC := $(wildcard tests/*.cpp)
LIB_OBJ  := $(patsubst %.cpp,build/%.o,$(LIB_SRC))
DEMO_OBJ := $(patsubst %.cpp,build/%.o,$(DEMO_SRC))
TEST_OBJ := $(patsubst %.cpp,build/%.o,$(TEST_SRC))

ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

APP  := build/ori$(EXE)
TEST := build/ori_tests$(EXE)

.PHONY: all lib test run clean

all: $(APP)

# Library only - useful while modules are being pushed one at a time.
lib: $(LIB_OBJ)

$(APP): $(LIB_OBJ) $(DEMO_OBJ) build/app/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST): $(LIB_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

test: $(TEST)
	./$(TEST)

run: $(APP)
	./$(APP)

clean:
	rm -rf build output

-include $(LIB_OBJ:.o=.d) $(DEMO_OBJ:.o=.d) $(TEST_OBJ:.o=.d) build/app/main.d
