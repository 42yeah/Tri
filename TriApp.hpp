#pragma once

#include "TriConfig.hpp"
#include "TriUtils.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// NOTE: This has to be the FIRST vulkan.hpp include
#define VULKAN_HPP_ASSERT_ON_RESULT(x)
#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>
#include <optional>

class TriApp
{
public:
    TriApp(int width, int height, const std::string &appName)
        : mWidth(width), mHeight(height), mAppName(appName), mpWindow(nullptr),
          mValid(false), mInstanceLayers(), mInstanceExtensions(),
          mDeviceExtensions(), mVkInstance()
    {
#if TRI_VK_FORCE_VALIDATION_LAYER
        mVkDebugUtilsMessengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT();
        mVkDebugUtilsMessenger = nullptr;
#endif
    }

    ~TriApp() { Finalize(); }

public:
    static VkBool32 OnDebugUtilsMessage(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);

public:
    ETriAppResult Init();
    void Finalize();

public:
    bool InitWindow();
    bool ChooseInstanceLayers();
    bool ChooseInstanceAndDeviceExtensions();
    bool InitVkInstance();

#if TRI_VK_FORCE_VALIDATION_LAYER
    bool InitDebugUtilsMessenger();
#endif

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

    vk::Instance mVkInstance;

#if TRI_VK_FORCE_VALIDATION_LAYER
    vk::DebugUtilsMessengerCreateInfoEXT mVkDebugUtilsMessengerCreateInfo;
    vk::DebugUtilsMessengerEXT mVkDebugUtilsMessenger;
#endif
};
