#include "UnrealProspero/Cook/SdkStatus.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/OutputDeviceNull.h"
#include <stdexcept>

namespace UnrealProspero::Cook
{
    bool SdkStatus::IsInstalled(const FString& platformName)
    {
        if (platformName.IsEmpty()) throw std::invalid_argument("SDK platform name is empty");
        for (TCHAR value : platformName)
        {
            if ((value < TEXT('A') || value > TEXT('Z')) && (value < TEXT('a') || value > TEXT('z')) && (value < TEXT('0') || value > TEXT('9'))) throw std::invalid_argument("SDK platform name contains invalid characters");
        }
        IDesktopPlatform* desktop = FDesktopPlatformModule::Get();
        if (!desktop || !desktop->IsUnrealBuildToolAvailable()) throw std::runtime_error("UnrealBuildTool is unavailable for SDK validation");
        FOutputDeviceNull log;
        int32 returnCode = -1;
        FString output;
        FString arguments = TEXT("-Mode=ValidatePlatforms -Platforms=") + platformName;
        if (!desktop->InvokeUnrealBuildToolSync(arguments, log, true, returnCode, output) || returnCode != 0) throw std::runtime_error("UnrealBuildTool SDK validation failed");
        return parse(platformName, output);
    }

    bool SdkStatus::parse(const FString& platformName, const FString& output)
    {
        FString prefix = TEXT("##PlatformValidate: ") + platformName + TEXT(" ");
        TArray<FString> lines;
        output.ParseIntoArrayLines(lines, false);
        bool found = false;
        bool installed = false;
        for (FString& line : lines)
        {
            line.TrimStartAndEndInline();
            if (!line.StartsWith(prefix, ESearchCase::CaseSensitive)) continue;
            if (found) throw std::runtime_error("UnrealBuildTool returned duplicate SDK validation results");
            FString status = line.Mid(prefix.Len()).TrimStartAndEnd();
            if (status == TEXT("VALID")) installed = true;
            else if (status != TEXT("INVALID")) throw std::runtime_error("UnrealBuildTool returned an invalid SDK status");
            found = true;
        }
        if (!found) throw std::runtime_error("UnrealBuildTool did not report the requested SDK status");
        return installed;
    }
}
