#include "TriUtils.hpp"

const char *TriAppResultToString(ETriAppResult result)
{
    switch (result)
    {
    case TriResultSuccess:
        return "Success";

    case TriResultWindow:
        return "Failed during GLFW window initialization";

    case TriResultLayerSelect:
        return "Failed during layer selection";
        
    case TriResultExtensionSelect:
        return "Failed during extension selection";

    case TriResultVkInstance:
        return "Failed during VkInstance creation";

    case TriResultVkDebugUtilsMessenger:
        return "Failed during VkDebugUtilsMessenger creation";

    case TriResultCount:
        return "Unknown failure";
    }

    return "";
}
