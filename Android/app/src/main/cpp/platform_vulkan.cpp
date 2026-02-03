
#include "platform_vulkan.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "GOEMON_VK", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GOEMON_VK", __VA_ARGS__)

static VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (auto f : formats) {
        if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    }
    return formats.front();
}

static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    for (auto m : modes) if (m == VK_PRESENT_MODE_FIFO_KHR)    return m; // guaranteed
    for (auto m : modes) if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

bool VkCreateInstanceForAndroid(VkInstance* outInstance, const std::vector<const char*>& extraExts) {
    std::vector<const char*> exts = extraExts;
    exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    exts.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);

    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Goemon64Recomp-Android";
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &appInfo;
    ici.enabledExtensionCount = (uint32_t)exts.size();
    ici.ppEnabledExtensionNames = exts.data();

#ifndef NDEBUG
    const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
    ici.enabledLayerCount = 1; // disable on release builds
    ici.ppEnabledLayerNames = layers;
#endif

    if (vkCreateInstance(&ici, nullptr, outInstance) != VK_SUCCESS) {
        LOGE("vkCreateInstance failed");
        return false;
    }
    return true;
}

bool CreateSurfaceFromSDL(SDL_Window* win, VkInstance instance, VkSurfaceKHR* outSurf) {
    if (!SDL_Vulkan_CreateSurface(win, instance, outSurf)) {
        LOGE("SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool CreateDeviceAndSwapchain(SDL_Window* win, PlatformVkDevice& dev) {
    dev.instance = VK_NULL_HANDLE;
    if (!VkCreateInstanceForAndroid(&dev.instance, {})) return false;
    if (!CreateSurfaceFromSDL(win, dev.instance, &dev.surface)) return false;

    // Pick physical device
    uint32_t physCount = 0;
    vkEnumeratePhysicalDevices(dev.instance, &physCount, nullptr);
    if (physCount == 0) { LOGE("No Vulkan physical devices"); return false; }
    std::vector<VkPhysicalDevice> phys(physCount);
    vkEnumeratePhysicalDevices(dev.instance, &physCount, phys.data());
    dev.phys = phys[0]; // TODO: choose best

    // Queue family selection
    uint32_t qCount = 0; vkGetPhysicalDeviceQueueFamilyProperties(dev.phys, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qprops(qCount); vkGetPhysicalDeviceQueueFamilyProperties(dev.phys, &qCount, qprops.data());
    uint32_t gfxFam = UINT32_MAX;
    for (uint32_t i=0;i<qCount;i++) {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev.phys, i, dev.surface, &present);
        if ((qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) { gfxFam = i; break; }
    }
    if (gfxFam == UINT32_MAX) { LOGE("No graphics+present queue family"); return false; }
    dev.graphicsQueueFamily = gfxFam;

    float prio = 1.0f;
    VkDeviceQueueCreateInfo dqci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    dqci.queueFamilyIndex = gfxFam; dqci.queueCount = 1; dqci.pQueuePriorities = &prio;

    const char* dExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &dqci;
    dci.enabledExtensionCount = 1; dci.ppEnabledExtensionNames = dExts;
    if (vkCreateDevice(dev.phys, &dci, nullptr, &dev.device) != VK_SUCCESS) {
        LOGE("vkCreateDevice failed"); return false;
    }
    vkGetDeviceQueue(dev.device, gfxFam, 0, &dev.graphicsQueue);

    // Swapchain
    VkSurfaceCapabilitiesKHR caps{}; vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev.phys, dev.surface, &caps);
    uint32_t fmtCount=0; vkGetPhysicalDeviceSurfaceFormatsKHR(dev.phys, dev.surface, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmtCount); vkGetPhysicalDeviceSurfaceFormatsKHR(dev.phys, dev.surface, &fmtCount, fmts.data());
    uint32_t pmCount=0; vkGetPhysicalDevicePresentModesKHR(dev.phys, dev.surface, &pmCount, nullptr);
    std::vector<VkPresentModeKHR> pms(pmCount); vkGetPhysicalDevicePresentModesKHR(dev.phys, dev.surface, &pmCount, pms.data());

    auto chosenFmt = ChooseSurfaceFormat(fmts);
    auto chosenPm  = ChoosePresentMode(pms);
    dev.surfaceFormat = chosenFmt.format;
    dev.colorSpace    = chosenFmt.colorSpace;
    dev.presentMode   = chosenPm;

    int w, h; SDL_Vulkan_GetDrawableSize(win, &w, &h);
    dev.extent = { (uint32_t)w, (uint32_t)h };

    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface = dev.surface;
    sci.minImageCount = std::max(caps.minImageCount, std::min(3u, caps.maxImageCount ? caps.maxImageCount : 3u));
    sci.imageFormat = dev.surfaceFormat;
    sci.imageColorSpace = dev.colorSpace;
    sci.imageExtent = dev.extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = dev.presentMode;
    sci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(dev.device, &sci, nullptr, &dev.swapchain) != VK_SUCCESS) {
        LOGE("vkCreateSwapchainKHR failed"); return false;
    }

    uint32_t ic = 0; vkGetSwapchainImagesKHR(dev.device, dev.swapchain, &ic, nullptr);
    dev.swapImages.resize(ic); vkGetSwapchainImagesKHR(dev.device, dev.swapchain, &ic, dev.swapImages.data());

    dev.swapViews.resize(ic);
    for (uint32_t i=0;i<ic;i++) {
        VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ivci.image = dev.swapImages[i];
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = dev.surfaceFormat;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        if (vkCreateImageView(dev.device, &ivci, nullptr, &dev.swapViews[i]) != VK_SUCCESS) {
            LOGE("vkCreateImageView failed"); return false;
        }
    }
    LOGI("Vulkan device & swapchain ready: %ux%u", dev.extent.width, dev.extent.height);
    return true;
}

void DestroySwapchain(PlatformVkDevice& dev) {
    for (auto v : dev.swapViews) if (v) vkDestroyImageView(dev.device, v, nullptr);
    dev.swapViews.clear();
    if (dev.swapchain) { vkDestroySwapchainKHR(dev.device, dev.swapchain, nullptr); dev.swapchain = VK_NULL_HANDLE; }
}

void DestroyDevice(PlatformVkDevice& dev) {
    DestroySwapchain(dev);
    if (dev.device) { vkDeviceWaitIdle(dev.device); vkDestroyDevice(dev.device, nullptr); dev.device = VK_NULL_HANDLE; }
    if (dev.surface) { vkDestroySurfaceKHR(dev.instance, dev.surface, nullptr); dev.surface = VK_NULL_HANDLE; }
    if (dev.instance) { vkDestroyInstance(dev.instance, nullptr); dev.instance = VK_NULL_HANDLE; }
}
