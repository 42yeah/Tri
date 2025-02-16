#include "TriApp.hpp"

#include "TriUtils.hpp"
#include "TriConfig.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_structs.hpp>

ETriAppResult TriApp::Init()
{
    if (!InitWindow())
        return TriResultWindow;

    if (!ChooseInstanceLayers())
        return TriResultLayerSelect;

    if (!ChooseInstanceAndDeviceExtensions())
        return TriResultExtensionSelect;

    return TriResultSuccess;
}

void TriApp::Finalize()
{
    mValid = false;
    
    mInstanceLayers.clear();
    mInstanceExtensions.clear();
    mDeviceExtensions.clear();
    
    FinalizeWindow();
}

bool TriApp::InitWindow()
{
    if (mpWindow)
        return true;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    mpWindow = glfwCreateWindow(mWidth, mHeight, mAppName.c_str(), nullptr, nullptr);
    
    return true;
}

bool TriApp::ChooseInstanceLayers()
{
    // Choose what extensions to use
#if TRI_VK_FORCE_VALIDATION_LAYER
    mInstanceLayers.emplace_back(TRI_VALIDATION_LAYER_NAME);

#endif

    // Iterate and locate layer availability
    std::vector<vk::LayerProperties> properties =
        vk::enumerateInstanceLayerProperties();
    std::vector<const char *> missingLayers;

    for (const char *layerName : mInstanceLayers)
    {
        auto pos = std::find_if(
            mInstanceLayers.begin(), mInstanceLayers.end(),
            [layerName](const vk::LayerProperties &prop)
            { return std::string(layerName) == prop.layerName.data(); });
        if (pos == mInstanceLayers.end())
            missingLayers.emplace_back(layerName);
    }

    // TODO(42): Print missing layers out

    return true;
}

bool TriApp::ChooseInstanceAndDeviceExtensions()
{
    return true;
}

void TriApp::FinalizeWindow()
{
    if (!mpWindow)
        return;

    glfwDestroyWindow(mpWindow);
    glfwTerminate();
    mpWindow = nullptr;
}
