#pragma once

#include "TriGraphicsUtils.hpp"
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
          mLibrary(), mDebugUtilsMessenger(nullptr), mPhysicalDevice(nullptr),
          mDevice(nullptr), mGraphicsQueue(nullptr), mPresentQueue(nullptr),
          mSurface(nullptr), mDeviceExtensions(), mSwapChain(nullptr)
    {
    }

    ~TriApp() { Finalize(); }

public:
    /* Initialize Vulkan-related stuffs:

       1. Create Vulkan instance
       2. Setup debug utils messenger
       3. Setup (pick) Vulkan physical device
       4. Setup logical Vulkan device
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
    void PopulateDebugUtilsMessengerCreateInfoEXT(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo);

    int RateDeviceSuitability(VkPhysicalDevice device,
                              const std::vector<const char *> &reqExtensions);

    QueueFamilyIndices FindQueueFamilies();

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats);

    VkPresentModeKHR
    ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &presentModes);

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

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

    VkPhysicalDevice mPhysicalDevice;
    QueueFamilyIndices mQueueFamilyIndices;

    VkDevice mDevice;
    VkQueue mGraphicsQueue;
    VkQueue mPresentQueue;

    VkSurfaceKHR mSurface;

    std::vector<VkExtensionProperties> mDeviceExtensions;

    VkSwapchainKHR mSwapChain;
};
