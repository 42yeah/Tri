#pragma once

#include "TriGraphicsUtils.hpp"
#include "TriConfig.hpp"
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
          mLibrary(), mPhysicalDevice(nullptr), mDevice(nullptr),
          mGraphicsQueue(nullptr), mPresentQueue(nullptr), mSurface(nullptr),
          mDeviceExtensions(), mSwapChain(nullptr), mSurfaceFormat(),
          mPresentMode(VK_PRESENT_MODE_FIFO_KHR), mSwapExtent(),
          mSwapChainImages(), mSwapChainImageViews(), mRenderPass(nullptr),
          mPipelineLayout(nullptr), mGraphicsPipeline(nullptr), mFramebuffers(),
          mCommandPool(nullptr), mCommandBuffer(nullptr),
          mImageAvailableSemaphore(nullptr), mRenderFinishedSemaphore(nullptr),
          mInFlightFence(nullptr)
    {
    #if TRI_WITH_VULKAN_VALIDATION
        mDebugUtilsMessenger = nullptr;
    #endif
    }

    ~TriApp() { Finalize(); }

public:
    /* Initialize Vulkan-related stuffs:

       1. Create Vulkan instance
       2. Setup debug utils messenger
       3. Setup swap surface
       4. Setup (pick) Vulkan physical device
       5. Setup logical Vulkan device
       6. Setup swap chains
       7. Setup swap chain image views
       8. Setup render pass
       9. Setup graphics pipeline
       10. Setup framebuffers
       11. Setup command buffer pool & command buffer
       12. Setup synchronization primitives
    */
    void Init();
    VkResult InitGraphicsPipeline();
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

    VkShaderModule CreateShaderModule(const std::vector<char> &svcBuffer);

    bool RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

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
    VkSurfaceFormatKHR mSurfaceFormat;
    VkPresentModeKHR mPresentMode;
    VkExtent2D mSwapExtent;
    std::vector<VkImage> mSwapChainImages;

    std::vector<VkImageView> mSwapChainImageViews;

    VkRenderPass mRenderPass;
    
    VkPipelineLayout mPipelineLayout;

    VkPipeline mGraphicsPipeline;

    std::vector<VkFramebuffer> mFramebuffers;

    VkCommandPool mCommandPool;
    VkCommandBuffer mCommandBuffer;

    // Synchronization primitives
    VkSemaphore mImageAvailableSemaphore;
    VkSemaphore mRenderFinishedSemaphore;
    VkFence mInFlightFence;
};
