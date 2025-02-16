#pragma once

#include "TriUtils.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>

class TriApp
{
public:
    TriApp(int width, int height, const std::string &appName)
        : mWidth(width), mHeight(height), mAppName(appName), mpWindow(nullptr),
          mValid(false), mInstanceLayers(), mInstanceExtensions(),
          mDeviceExtensions()
    {
    }
    
    ~TriApp()
    {
        Finalize();
    }

public:
    ETriAppResult Init();
    void Finalize();

public:
    bool InitWindow();
    bool ChooseInstanceLayers();
    bool ChooseInstanceAndDeviceExtensions();

    void FinalizeWindow();

private:
    int mWidth;
    int mHeight;
    std::string mAppName;
    GLFWwindow *mpWindow;

    // An app is only valid when initialization is successfully done
    bool mValid;

    // Layers & extensions
    std::vector<const char *> mInstanceLayers;
    std::vector<const char *> mInstanceExtensions;
    std::vector<const char *> mDeviceExtensions;
};

