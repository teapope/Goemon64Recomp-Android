
#pragma once
#include "platform_vulkan.h"
#include <cstdint>

// Bridge to RT64 (Vulkan via plume). Implement these by calling RT64 APIs.
bool RT64_InitWithVulkan(const PlatformVkDevice& dev);
void RT64_OnResize(uint32_t w, uint32_t h);
void RT64_RenderFrame();
void RT64_Shutdown();
