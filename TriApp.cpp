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

    if (!mInstance)
    {
        // Create Vulkan instance
        VkApplicationInfo appInfo;
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = mAppName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.pApplicationInfo = &appInfo;

        // Get extensions required from GLFW
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions =
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        TriLogVerbose() << "Number of GLFW extension count: "
            << glfwExtensionCount;
        for (size_t i = 0; i < glfwExtensionCount; i++)
        {
            TriLogVerbose() << "- " << glfwExtensions[i];
        }

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;

        VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create VkInstance: " << result;
            Finalize();
            return;
        }

        TriLogVerbose() << "VkInstance created";
    }

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
    }

#if TRI_WITH_VULKAN_VALIDATION
    if (mInstanceLayers.empty())
    {
        // Req. layers
        std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"};

        uint32_t numLayers = 0;
        vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
        mInstanceLayers.resize(numLayers);
        vkEnumerateInstanceLayerProperties(&numLayers, mInstanceLayers.data());

        TriLogInfo() << "Number of available layers: " << numLayers;

        for (size_t i = 0; i < numLayers; i++)
        {
            TriLogVerbose() << "- " << mInstanceLayers[i].layerName;
        }

        for (const char *reqLayer : validationLayers)
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

#endif
    }
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
    if (mInstance)
    {
        vkDestroyInstance(mInstance, nullptr);
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

void TriApp::RenderFrame() {}
