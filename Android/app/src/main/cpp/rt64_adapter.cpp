
#include "rt64_adapter.h"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "GOEMON_RT64", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GOEMON_RT64", __VA_ARGS__)

// TODO: include RT64/plume headers and wire up to its initialization API.

bool RT64_InitWithVulkan(const PlatformVkDevice& dev) {
    LOGI("[RT64] Init (placeholder) with Vulkan device & swapchain...");
    // Call into RT64 to create renderer using dev.instance/device/queue/surface/swapchain.
    return true; // return false on error
}

void RT64_OnResize(uint32_t w, uint32_t h) {
    LOGI("[RT64] Resize to %ux%u (placeholder)", w, h);
}

void RT64_RenderFrame() {
    // Invoke RT64 frame render (placeholder). For now, no-op.
}

void RT64_Shutdown() {
    LOGI("[RT64] Shutdown (placeholder)");
}
