#pragma once

#include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

class TriApp
{
public:
    TriApp(const std::string &appName, int width, int height)
        : mpWindow(nullptr), mAppName(appName), width(width), height(height),
            mInstance(nullptr), mInstanceExtensions(), mInstanceLayers()
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
};
