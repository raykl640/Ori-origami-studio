# ORI - Procedural Origami Folding Studio
#   cmake -S . -B build && cmake --build build --config Release
#   ./build/ori            (Windows: build\Release\ori.exe)
cmake_minimum_required(VERSION 3.14)
project(ori_origami_studio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# One static library per checkpoint: every module of src/
file(GLOB ORI_LIB_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
add_library(ori_lib STATIC ${ORI_LIB_SOURCES})
target_include_directories(ori_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The app and the tests are added once their folders have been pushed, so the
# library still builds while members are adding their modules one by one.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/demos/Registry.cpp)
  file(GLOB ORI_DEMO_SOURCES CONFIGURE_DEPENDS demos/*.cpp)
  add_executable(ori app/main.cpp ${ORI_DEMO_SOURCES})
  target_link_libraries(ori PRIVATE ori_lib)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp)
  file(GLOB ORI_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
  add_executable(ori_tests ${ORI_TEST_SOURCES})
  target_link_libraries(ori_tests PRIVATE ori_lib)
  enable_testing()
  add_test(NAME unit_tests COMMAND ori_tests)
endif()
