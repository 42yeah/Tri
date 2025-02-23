#include "TriApp.hpp"

#include "TriLog.hpp"
#include "TriUtils.hpp"
#include "TriConfig.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

VkBool32 TriApp::OnDebugUtilsMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    // TriApp *that = reinterpret_cast<TriApp *>(pUserData);
    
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
        TriLogError() << "[VK ] Unknown message type: "
            << pCallbackData->pMessage;
        break;
    }
    
    return false;
}

ETriAppResult TriApp::Init()
{
    if (!InitWindow())
        return TriResultWindow;

    if (!ChooseInstanceLayers())
        return TriResultLayerSelect;

    if (!ChooseInstanceAndDeviceExtensions())
        return TriResultExtensionSelect;

    if (!InitVkInstance())
        return TriResultVkInstance;

    if (!InitDebugUtilsMessenger())
        return TriResultVkDebugUtilsMessenger;

    return TriResultSuccess;
}

void TriApp::Finalize()
{
    TriLogInfo() << "Finalize";
    
    mValid = false;

    if (mVkDebugUtilsMessenger)
    {
        mVkInstance.destroyDebugUtilsMessengerEXT(
            mVkDebugUtilsMessenger, nullptr, mDispatchLoaderDynamic);
        
        mVkDebugUtilsMessenger = nullptr;
    }
    
    if (mVkInstance)
    {
        mVkInstance.destroy();
        mVkInstance = nullptr;
    }

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
        vk::enumerateInstanceLayerProperties().value;
    std::vector<const char *> missingLayers;

    for (const char *layerName : mInstanceLayers)
    {
        auto pos = std::find_if(
            properties.begin(), properties.end(),
            [layerName](const vk::LayerProperties &prop)
            { return std::string(layerName) == prop.layerName.data(); });
        bool found = (pos != properties.end());
        if (!found)
            missingLayers.emplace_back(layerName);

        TriLogVerbose() << "Instance layer support: " << layerName
            << " .. " << (found ? "yes" : "no");
    }

    for (const char *layerName : missingLayers)
        TriLogError() << "Missing layer: " << layerName;
    
    return missingLayers.empty();
}

bool TriApp::ChooseInstanceAndDeviceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **ppGlfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    mInstanceExtensions.insert(mInstanceExtensions.end(), ppGlfwExtensions,
                               ppGlfwExtensions + glfwExtensionCount);

#if TRI_VK_FORCE_VALIDATION_LAYER
    mInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#endif

    std::vector<vk::ExtensionProperties> properties =
        vk::enumerateInstanceExtensionProperties().value;
    std::vector<const char *> missingExtensions;

    for (const char *extName : mInstanceExtensions)
    {
        auto pos = std::find_if(
            properties.begin(), properties.end(),
            [extName](const vk::ExtensionProperties &prop)
            { return std::string(extName) == prop.extensionName.data(); });
        bool found = (pos != properties.end());
        if (!found)
            missingExtensions.emplace_back(extName);

        TriLogVerbose() << "Extension support: " << extName << "  .. "
            << (found ? "yes" : "no");
    }

    for (const char *extName : missingExtensions)
        TriLogError() << "Missing extensions: " << extName;

    // TODO(42): Device extensions
    
    return missingExtensions.empty();
}

bool TriApp::InitVkInstance()
{
#if TRI_VK_FORCE_VALIDATION_LAYER
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT typeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding);

    mVkDebugUtilsMessengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT(
        {}, severityFlags, typeFlags, TriApp::OnDebugUtilsMessage, this,
        nullptr);

#endif

    vk::InstanceCreateInfo createInfo =
        vk::InstanceCreateInfo()
#if TRI_VK_FORCE_VALIDATION_LAYER
            .setPNext(&mVkDebugUtilsMessengerCreateInfo)
#endif
            .setPEnabledLayerNames(mInstanceLayers)
            .setPEnabledExtensionNames(mInstanceExtensions);

    vk::ResultValue<vk::Instance> instance = vk::createInstance(createInfo);

    if (instance.result != vk::Result::eSuccess)
        return false;

    mVkInstance = std::move(instance.value);

    VkInstance cInstance = static_cast<VkInstance>(mVkInstance);
    mDispatchLoaderDynamic =
        vk::detail::DispatchLoaderDynamic(cInstance, vkGetInstanceProcAddr);

    return true;
}

#if TRI_VK_FORCE_VALIDATION_LAYER
bool TriApp::InitDebugUtilsMessenger()
{
    vk::ResultValue<vk::DebugUtilsMessengerEXT> messenger = mVkInstance.createDebugUtilsMessengerEXT(
        mVkDebugUtilsMessengerCreateInfo, nullptr, mDispatchLoaderDynamic);

    if (messenger.result != vk::Result::eSuccess)
        return false;

    mVkDebugUtilsMessenger = messenger.value;

    return true;
}
#endif

void TriApp::FinalizeWindow()
{
    if (!mpWindow)
        return;

    glfwDestroyWindow(mpWindow);
    glfwTerminate();
    mpWindow = nullptr;
}
