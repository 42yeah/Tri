#pragma once

#include "VkExtLibrary.hpp"

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

class TriApp
{
public:
    TriApp(const std::string &appName, int width, int height)
        : mpWindow(nullptr), mAppName(appName), width(width), height(height),
          mInstance(nullptr), mInstanceExtensions(), mInstanceLayers(),
          mLibrary(), mDebugUtilsMessenger(nullptr)
    {
    }

    ~TriApp() { Finalize(); }

public:
    /* Initialize Vulkan-related stuffs:

       1. Create Vulkan instance
       
    */
    void Init();
    void Loop();
    void Finalize();

public:
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    VKDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                    VkDebugUtilsMessageTypeFlagsEXT type,
                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                    void *pUserData);

private:
    void RenderFrame();

private:
    // UI
    GLFWwindow *mpWindow;

    std::string mAppName;
    int width;
    int height;

private:
    // Vulkan
    VkInstance mInstance;
    std::vector<VkExtensionProperties> mInstanceExtensions;
    std::vector<VkLayerProperties> mInstanceLayers;

    VkExtLibary mLibrary;

#if TRI_WITH_VULKAN_VALIDATION
    VkDebugUtilsMessengerEXT mDebugUtilsMessenger;
#endif
};
