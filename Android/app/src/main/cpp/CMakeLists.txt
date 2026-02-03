
cmake_minimum_required(VERSION 3.22)
project(goemon64recomp_android LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_ANDROID_STL_TYPE c++_static)

# SDL2 (must be present at third_party/SDL2)
# Build SDL with Vulkan support
add_subdirectory(${CMAKE_SOURCE_DIR}/../../../../third_party/SDL2 SDL2_build)
target_compile_definitions(SDL2-static PUBLIC SDL_VIDEO_VULKAN=1)

# NDK libs
find_library(log-lib log)
find_library(android-lib android)
find_library(vulkan-lib vulkan)

# --- Goemon + RT64 sources ---
# Replace these with explicit file lists from Goemon64Recomp once added
file(GLOB_RECURSE GOEMON_SRC
    ${CMAKE_SOURCE_DIR}/../../../../src/*.cpp
    ${CMAKE_SOURCE_DIR}/../../../../lib/**/*.cpp
)

add_library(goemon64recomp SHARED
    ${GOEMON_SRC}
    platform_vulkan.cpp
    sdl_app.cpp
    audio_sdl.cpp
    rt64_adapter.cpp
)

target_include_directories(goemon64recomp PRIVATE
    ${CMAKE_SOURCE_DIR}/../../../../include
    ${CMAKE_SOURCE_DIR}/../../../../third_party/rt64/include
)

target_link_libraries(goemon64recomp
    SDL2-static
    ${vulkan-lib}
    ${android-lib}
    ${log-lib}
)
