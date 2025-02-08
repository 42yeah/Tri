#include "TriApp.hpp"

#include "TriLog.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>

void TriApp::Init()
{
    if (!mpWindow)
    {
        glfwInit();
        mpWindow = glfwCreateWindow(width, height, mAppName.c_str(), nullptr, nullptr);
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
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

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
    if (mpWindow)
    {
        glfwTerminate();
        mpWindow = nullptr;
    }
}

void TriApp::RenderFrame()
{
    
}
