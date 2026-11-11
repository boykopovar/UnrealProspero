#include "UnrealProspero/Runnable/Priority.h"
#include <stdexcept>

namespace UnrealProspero::Runnable
{
    int GetKernelPriority(EThreadPriority priority)
    {
        switch (priority)
        {
            case TPri_TimeCritical: return 256;
            case TPri_Highest: return 512;
            case TPri_AboveNormal: return 600;
            case TPri_Normal: return 700;
            case TPri_SlightlyBelowNormal: return 720;
            case TPri_BelowNormal: return 740;
            case TPri_Lowest: return 767;
            default: throw std::invalid_argument("Invalid Unreal thread priority");
        }
    }
}
