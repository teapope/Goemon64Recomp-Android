
#include <SDL.h>
#include <SDL_vulkan.h>
#include <android/log.h>

#include "platform_vulkan.h"
#include "rt64_adapter.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "GOEMON_APP", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GOEMON_APP", __VA_ARGS__)

static PlatformVkDevice gDev;

static bool InitRenderer(SDL_Window* win) {
    if (!CreateDeviceAndSwapchain(win, gDev)) return false;
    if (!RT64_InitWithVulkan(gDev)) return false;
    return true;
}

static void ShutdownRenderer() {
    RT64_Shutdown();
    DestroyDevice(gDev);
}

extern "C" int SDL_main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR | SDL_INIT_AUDIO) != 0) {
        LOGE("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }
    SDL_Window* win = SDL_CreateWindow("Goemon 64",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!win) { LOGE("SDL_CreateWindow failed: %s", SDL_GetError()); return -2; }

    if (!InitRenderer(win)) { LOGE("Renderer init failed"); return -3; }

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                RT64_OnResize((uint32_t)e.window.data1, (uint32_t)e.window.data2);
            }
            // TODO: map SDL input events to Goemon input system
        }
        // TODO: call game tick/update
        RT64_RenderFrame();
        SDL_Delay(0); // Yield; use proper frame pacing later
    }
    ShutdownRenderer();
    SDL_Quit();
    return 0;
}
