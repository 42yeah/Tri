#include "TriApp.hpp"

#include "TriLog.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>

#include <cstdint>

void TriApp::Init()
{
    if (!mpWindow)
    {
        glfwInit();
        mpWindow =
            glfwCreateWindow(width, height, mAppName.c_str(), nullptr, nullptr);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwMakeContextCurrent(mpWindow);
    }

    std::vector<const char *> reqExtensions;

    if (mInstanceExtensions.empty())
    {
        uint32_t numInstanceExtensions = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions,
                                               nullptr);
        mInstanceExtensions.resize(numInstanceExtensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions,
                                               mInstanceExtensions.data());

        TriLogInfo() << "Number of available instance extensions: "
                     << numInstanceExtensions;

        for (size_t i = 0; i < numInstanceExtensions; i++)
        {
            TriLogVerbose() << "- " << mInstanceExtensions[i].extensionName;
        }

        // Get extensions required from GLFW
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions =
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        reqExtensions =
            std::vector(glfwExtensions, glfwExtensions + glfwExtensionCount);
#if TRI_WITH_VULKAN_VALIDATION
        reqExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        for (const char *reqExtension : reqExtensions)
        {
            auto position = std::find_if(
                mInstanceExtensions.begin(), mInstanceExtensions.end(),
                [&](const VkExtensionProperties &prop)
                { return std::string(reqExtension) == prop.extensionName; });

            if (position == mInstanceExtensions.end())
            {
                TriLogError() << "Missing instance extension: " << reqExtension;
                Finalize();
                return;
            }
        }

        TriLogInfo() << "All required extensions found";
    }

    std::vector<const char *> reqLayers;

    if (mInstanceLayers.empty())
    {
#if TRI_WITH_VULKAN_VALIDATION
        // Req. layers
        reqLayers.emplace_back("VK_LAYER_KHRONOS_validation");

#endif

        uint32_t numLayers = 0;
        vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
        mInstanceLayers.resize(numLayers);
        vkEnumerateInstanceLayerProperties(&numLayers, mInstanceLayers.data());

        TriLogInfo() << "Number of available layers: " << numLayers;

        for (size_t i = 0; i < numLayers; i++)
        {
            TriLogVerbose() << "- " << mInstanceLayers[i].layerName;
        }

        for (const char *reqLayer : reqLayers)
        {
            auto position = std::find_if(
                mInstanceLayers.begin(), mInstanceLayers.end(),
                [&](const VkLayerProperties &prop)
                { return std::string(reqLayer) == prop.layerName; });

            if (position == mInstanceLayers.end())
            {
                TriLogError() << "Missing required layer: " << reqLayer;
                Finalize();
                return;
            }
        }

        TriLogInfo() << "All required layers found";
    }

    if (!mInstance)
    {
        // Create Vulkan instance
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = mAppName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.pApplicationInfo = &appInfo;

        TriLogInfo() << "Number of requested instance extensions: "
                     << reqExtensions.size();
        for (size_t i = 0; i < reqExtensions.size(); i++)
        {
            TriLogVerbose() << "- " << reqExtensions[i];
        }

        TriLogInfo() << "Number of requested instance layers: "
                     << reqLayers.size();
        for (size_t i = 0; i < reqLayers.size(); i++)
        {
            TriLogVerbose() << "- " << reqLayers[i];
        }

        createInfo.enabledExtensionCount = reqExtensions.size();
        createInfo.ppEnabledExtensionNames = reqExtensions.data();
        createInfo.enabledLayerCount = reqLayers.size();
        createInfo.ppEnabledLayerNames = reqLayers.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create VkInstance: " << result;
            Finalize();
            return;
        }

        TriLogVerbose() << "VkInstance created: " << mInstance;
    }

    mLibrary.Init();

#if TRI_WITH_VULKAN_VALIDATION
    if (!mDebugUtilsMessenger)
    {

        // Setup VkDebugUtilsMessenger
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.pNext = nullptr;

        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        createInfo.pfnUserCallback = TriApp::VKDebugCallback;
        createInfo.pUserData = this;

        VkResult result = mLibrary.CreateDebugUtilsMessengerEXT(
            mInstance, &createInfo, nullptr, &mDebugUtilsMessenger);

        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create Vulkan DebugUtilsMessenger - "
                             "there will be no messages from validation layer";
        }
    }
#endif
}

void TriApp::Loop()
{
    while (!glfwWindowShouldClose(mpWindow))
    {
        glfwPollEvents();
        RenderFrame();
        glfwSwapBuffers(mpWindow);
    }
}

void TriApp::Finalize()
{
    if (mDebugUtilsMessenger)
    {
        mLibrary.DestroyDebugUtilsMessengerEXT(mInstance, mDebugUtilsMessenger,
                                               nullptr);
        mDebugUtilsMessenger = nullptr;
    }

    mLibrary.Finalize();

    if (mInstance)
    {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = nullptr;
    }

    if (mpWindow)
    {
        glfwDestroyWindow(mpWindow);
        glfwTerminate();
        mpWindow = nullptr;
    }

    mInstanceExtensions.clear();
    mInstanceLayers.clear();
}

VKAPI_ATTR VkBool32 VKAPI_CALL TriApp::VKDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    // TriApp *that = static_cast<TriApp *>(pUserData);

    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        TriLogVerbose() << "[VK] " << pCallbackData->pMessage;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        TriLogInfo() << "[VK] " << pCallbackData->pMessage;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        TriLogWarning() << "[VK] " << pCallbackData->pMessage;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        TriLogError() << "[VK] " << pCallbackData->pMessage;
        break;

    default:
        TriLogWarning() << "[VK] Unknown severity: " << pCallbackData->pMessage;
        break;
    }

    return VK_FALSE;
}

void TriApp::RenderFrame() {}
