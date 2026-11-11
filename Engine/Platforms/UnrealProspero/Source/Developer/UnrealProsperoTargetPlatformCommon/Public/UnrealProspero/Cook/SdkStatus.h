#ifndef UNREALPROSPERO_COOK_SDK_STATUS_H
#define UNREALPROSPERO_COOK_SDK_STATUS_H

#include "Containers/UnrealString.h"

namespace UnrealProspero::Cook
{
    class UNREALPROSPEROTARGETPLATFORMCOMMON_API SdkStatus final
    {
    public:
        static bool IsInstalled(const FString& platformName);

    private:
        static bool parse(const FString& platformName, const FString& output);
    };
}

#endif
