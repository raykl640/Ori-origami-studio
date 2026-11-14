# ORI - Procedural Origami Folding Studio
#   cmake -S . -B build && cmake --build build --config Release
#   ctest --test-dir build -C Release        (unit tests)
#   ./build/ori                              (Windows: build\Release\ori.exe)
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

# The library: every module of src/ (one folder per module and owner).
file(GLOB ORI_LIB_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
if(NOT ORI_LIB_SOURCES)
  message(WARNING "No sources in src/ yet - run scripts/ci_check.sh status to see which packages are missing.")
  return()
endif()
add_library(ori_lib STATIC ${ORI_LIB_SOURCES})
target_include_directories(ori_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The app, the studio and the tests are added once their folders exist, so
# the library can still be configured while packages are being pushed.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/demos/Registry.cpp AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/studio/Studio.cpp)
  # Demos + studio UI (no windowing code) - shared by ori, ori_studio and the tests.
  file(GLOB ORI_DEMO_SOURCES CONFIGURE_DEPENDS demos/*.cpp)
  file(GLOB ORI_STUDIO_SOURCES CONFIGURE_DEPENDS studio/*.cpp)
  add_library(ori_ui STATIC ${ORI_DEMO_SOURCES} ${ORI_STUDIO_SOURCES})
  target_link_libraries(ori_ui PUBLIC ori_lib)

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp)
    add_executable(ori app/main.cpp)
    target_link_libraries(ori PRIVATE ori_ui)
  endif()

  # The interactive studio needs SDL2 only to open a window; it is skipped
  # (with a message) when SDL2 is not installed.
  set(ORI_SDL_TARGET "")
  find_package(SDL2 CONFIG QUIET)
  if(TARGET SDL2::SDL2)
    set(ORI_SDL_TARGET SDL2::SDL2)
  else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(ORI_SDL2 QUIET IMPORTED_TARGET sdl2)
      if(TARGET PkgConfig::ORI_SDL2)
        set(ORI_SDL_TARGET PkgConfig::ORI_SDL2)
      endif()
    endif()
  endif()
  if(ORI_SDL_TARGET AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/studio/sdl/main.cpp)
    add_executable(ori_studio studio/sdl/main.cpp)
    target_link_libraries(ori_studio PRIVATE ori_ui ${ORI_SDL_TARGET})
  else()
    message(STATUS "SDL2 not found - building without ori_studio (install libsdl2-dev / sdl2)")
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp)
    file(GLOB ORI_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
    add_executable(ori_tests ${ORI_TEST_SOURCES})
    target_link_libraries(ori_tests PRIVATE ori_ui)
    enable_testing()
    add_test(NAME unit_tests COMMAND ori_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endif()
