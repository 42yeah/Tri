#pragma once

#define TRI_VALIDATION_LAYER_NAME "VK_LAYER_KHORNOS_validation"

enum ETriAppResult
{
    TriResultSuccess = 0,
    TriResultWindow,
    TriResultLayerSelect,
    TriResultExtensionSelect,
    TriResultCount
};

const char *TriAppResultToString(ETriAppResult result);

