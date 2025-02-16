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

    case TriResultCount:
        return "Unknown failure";
    }

    return "";
}
