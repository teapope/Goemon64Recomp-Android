
#include "rt64_adapter.h"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "GOEMON_RT64", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GOEMON_RT64", __VA_ARGS__)

#ifndef GOEMON_ANDROID_NO_RT64
// TODO: Replace these includes with your RT64 fork's actual headers
// #include <rt64/Renderer.h>
// #include <rt64/Swapchain.h>
// #include <rt64/PlumeBackend.h>

static bool g_rt64Ready = false;

bool RT64_InitWithVulkan(const PlatformVkDevice& dev) {
    // TODO: Create/attach RT64 renderer using dev.instance/dev.device/dev.graphicsQueue/dev.surface/dev.swapchain
    // Configure no-MSAA, preferred present mode, and set initial viewport to dev.extent
    g_rt64Ready = true;
    LOGI("[RT64] Init with Vulkan (device + swapchain) [placeholder]");
    return true;
}

void RT64_OnResize(uint32_t w, uint32_t h) {
    if (!g_rt64Ready) return;
    // TODO: Rebuild RT64 swapchain/framebuffers for new size (w,h)
    LOGI("[RT64] Resize to %ux%u (placeholder)", w, h);
}

void RT64_RenderFrame() {
    if (!g_rt64Ready) return;
    // TODO: Acquire-present via RT64 and submit the scene from Goemon
}

void RT64_Shutdown() {
    if (!g_rt64Ready) return;
    // TODO: Destroy RT64 renderer and associated resources
    g_rt64Ready = false;
    LOGI("[RT64] Shutdown");
}

#else
// --- No-RT64 build (compiles, renders nothing) ---
bool RT64_InitWithVulkan(const PlatformVkDevice&) { LOGI("[RT64] not present; running stub"); return true; }
void RT64_OnResize(uint32_t, uint32_t) {}
void RT64_RenderFrame() {}
void RT64_Shutdown() {}
#endif
