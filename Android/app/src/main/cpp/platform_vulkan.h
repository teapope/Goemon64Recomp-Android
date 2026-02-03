
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

struct PlatformVkDevice {
    VkInstance instance{};
    VkPhysicalDevice phys{};
    VkDevice device{};
    uint32_t graphicsQueueFamily = 0;
    VkQueue graphicsQueue{};
    VkSurfaceKHR surface{};
    VkFormat surfaceFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkSwapchainKHR swapchain{};
    VkExtent2D extent{};
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;
    uint32_t imageCount = 0;
};

bool VkCreateInstanceForAndroid(VkInstance* outInstance,
                                const std::vector<const char*>& extraExts = {});
bool CreateSurfaceFromSDL(struct SDL_Window* win, VkInstance instance, VkSurfaceKHR* outSurf);
bool CreateDeviceAndSwapchain(struct SDL_Window* win, PlatformVkDevice& dev);
void DestroySwapchain(PlatformVkDevice& dev);
void DestroyDevice(PlatformVkDevice& dev);
