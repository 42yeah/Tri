#pragma once

#include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

class TriApp
{
public:
    TriApp(const std::string &appName, int width, int height)
        : mpWindow(nullptr), mAppName(appName), width(width), height(height),
            mInstance(nullptr)
    {
    }

    ~TriApp() { Finalize(); }

public:
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
    
};
