#pragma once

#define TRI_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

enum ETriAppResult
{
    TriResultSuccess = 0,
    TriResultWindow,
    TriResultLayerSelect,
    TriResultExtensionSelect,
    TriResultVkInstance,
    TriResultVkDebugUtilsMessenger,
    TriResultCount
};

const char *TriAppResultToString(ETriAppResult result);

