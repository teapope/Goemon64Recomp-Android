
cmake_minimum_required(VERSION 3.22)
project(goemon64recomp_android LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_ANDROID_STL_TYPE c++_static)

# SDL2
add_subdirectory(${CMAKE_SOURCE_DIR}/../../../../third_party/SDL2 SDL2_build)
target_compile_definitions(SDL2-static PUBLIC SDL_VIDEO_VULKAN=1)

# NDK libs
find_library(log-lib log)
find_library(android-lib android)
find_library(vulkan-lib vulkan)

# --- RT64 autodiscovery ---
set(RT64_FOUND OFF)
if(DEFINED RT64_DIR)
    set(_rt64_candidates "${RT64_DIR}")
else()
    set(_rt64_candidates
        ${CMAKE_SOURCE_DIR}/../../../../third_party/rt64
        ${CMAKE_SOURCE_DIR}/../../../../external/rt64
        ${CMAKE_SOURCE_DIR}/../../../../lib/rt64
        ${CMAKE_SOURCE_DIR}/../../../../rt64
        ${CMAKE_SOURCE_DIR}/../../../../Goemon64Recomp/third_party/rt64
        ${CMAKE_SOURCE_DIR}/../../../../Goemon64Recomp/lib/rt64
    )
endif()
foreach(_cand ${_rt64_candidates})
    if(EXISTS "${_cand}")
        set(RT64_DIR "${_cand}")
        set(RT64_FOUND ON)
        break()
    endif()
endforeach()
if(NOT RT64_FOUND)
    message(WARNING "RT64 not found. Set -DRT64_DIR=/path/to/rt64. Building without RT64 will stub rendering.")
endif()

# --- Goemon + platform sources ---
# Replace these with explicit lists from Goemon64Recomp
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
)

if(RT64_FOUND)
    if(EXISTS "${RT64_DIR}/CMakeLists.txt")
        message(STATUS "RT64: using CMake project at ${RT64_DIR}")
        add_subdirectory(${RT64_DIR} rt64_build)
        if(TARGET RT64::rt64)
            target_link_libraries(goemon64recomp PRIVATE RT64::rt64)
        elseif(TARGET rt64)
            target_link_libraries(goemon64recomp PRIVATE rt64)
        endif()
        if(EXISTS "${RT64_DIR}/include")
            target_include_directories(goemon64recomp PRIVATE ${RT64_DIR}/include)
        endif()
    else()
        message(STATUS "RT64: treating ${RT64_DIR} as sources-only")
        if(EXISTS "${RT64_DIR}/include")
            target_include_directories(goemon64recomp PRIVATE ${RT64_DIR}/include)
        endif()
        file(GLOB_RECURSE RT64_SRC
            ${RT64_DIR}/src/*.cpp
            ${RT64_DIR}/src/*.c
        )
        if(RT64_SRC)
            target_sources(goemon64recomp PRIVATE ${RT64_SRC})
        else()
            message(WARNING "RT64 sources not found under ${RT64_DIR}/src; adapter may fail to link")
        endif()
    endif()
else()
    target_compile_definitions(goemon64recomp PRIVATE GOEMON_ANDROID_NO_RT64=1)
endif()

# Link order
target_link_libraries(goemon64recomp
    SDL2-static
    ${vulkan-lib}
    ${android-lib}
    ${log-lib}
)
