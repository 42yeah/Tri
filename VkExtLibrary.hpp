#pragma once

#include "TriLog.hpp"
#include "vulkan/vulkan_core.h"

#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>

#define VK_EXT_FUNC_ITERATE(funcName, params, args)                            \
    VkResult funcName params                                                   \
    {                                                                          \
        void *func = mFuncs[#funcName];                                        \
        if (!func)                                                             \
        {                                                                      \
            func = reinterpret_cast<void *>(                                   \
                vkGetInstanceProcAddr(instance, "vk" #funcName));              \
            if (!func)                                                         \
            {                                                                  \
                TriLogError()                                                  \
                    << "Extension function not found: " << "vk" #funcName;     \
                return VK_ERROR_EXTENSION_NOT_PRESENT;                         \
            }                                                                  \
            mFuncs[#funcName] = func;                                          \
        }                                                                      \
        return reinterpret_cast<PFN_vk##funcName>(func) args;                  \
    }

#define VK_EXT_FUNC_ITERATE_VOID(funcName, params, args)                       \
    void funcName params                                                       \
    {                                                                          \
        void *func = mFuncs[#funcName];                                        \
        if (!func)                                                             \
        {                                                                      \
            func = reinterpret_cast<void *>(                                   \
                vkGetInstanceProcAddr(instance, "vk" #funcName));              \
            if (!func)                                                         \
            {                                                                  \
                TriLogError()                                                  \
                    << "Extension function not found: " << "vk" #funcName;     \
                return;                                                        \
            }                                                                  \
            mFuncs[#funcName] = func;                                          \
        }                                                                      \
        reinterpret_cast<PFN_vk##funcName>(func) args;                         \
    }

// Class that loads Vulkan extension functions

class VkExtLibary
{
public:
    VkExtLibary() : mFuncs() {}

    ~VkExtLibary() { Finalize(); }

public:
    void Init();
    void Finalize();

public:
    VK_EXT_FUNC_ITERATE(
        CreateDebugUtilsMessengerEXT,
        (VkInstance instance,
         const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
         const VkAllocationCallbacks *pAllocator,
         VkDebugUtilsMessengerEXT *pDebugMessenger),
         (instance, pCreateInfo, pAllocator, pDebugMessenger));

    VK_EXT_FUNC_ITERATE_VOID(DestroyDebugUtilsMessengerEXT,
                        (VkInstance instance,
                         VkDebugUtilsMessengerEXT debugMessenger,
                         const VkAllocationCallbacks *pAllocator),
                         (instance, debugMessenger, pAllocator));

private:
    std::unordered_map<std::string, void *> mFuncs;
};
