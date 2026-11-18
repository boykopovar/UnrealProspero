#include "UnrealProspero/RHI/Device/ShaderPlatform.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIGlobals.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    EShaderPlatform ResolveShaderPlatform(Shader::AgcTarget target)
    {
        const TCHAR* name;
        const TCHAR* format;
        switch (target)
        {
        case Shader::AgcTarget::Windows: name = TEXT("SP_AGC_ANYWIN64"); format = TEXT("SF_AGC_ANYWIN64"); break;
        case Shader::AgcTarget::Linux: name = TEXT("SP_AGC_ANYLINUX"); format = TEXT("SF_AGC_ANYLINUX"); break;
        case Shader::AgcTarget::Ps5: name = TEXT("SP_AGC_ANYPS5"); format = TEXT("SF_AGC_ANYPS5"); break;
        default: throw std::invalid_argument("Invalid shader platform target");
        }
        const auto platform = FDataDrivenShaderPlatformInfo::GetShaderPlatformFromName(FName(name));
        if (platform >= SP_NumPlatforms || !FDataDrivenShaderPlatformInfo::IsValid(platform)) throw std::runtime_error("UnrealProspero shader platform is not registered");
        if (FDataDrivenShaderPlatformInfo::GetShaderFormat(platform) != FName(format)) throw std::runtime_error("UnrealProspero shader format does not match its target");
        if (FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(platform) != ERHIFeatureLevel::SM5) throw std::runtime_error("UnrealProspero requires the SM5 shader platform");
        if (!FDataDrivenShaderPlatformInfo::GetNeedsOfflineCompiler(platform)) throw std::runtime_error("UnrealProspero requires offline shader compilation");
        if (!RHISupportsShaderPipelines(platform)) throw std::runtime_error("UnrealProspero requires offline shader pipelines");
        return platform;
    }

    void ConfigureShaderPlatform(EShaderPlatform platform)
    {
        if (platform >= SP_NumPlatforms || !FDataDrivenShaderPlatformInfo::IsValid(platform)) throw std::invalid_argument("Invalid shader platform configuration");
        if (FDataDrivenShaderPlatformInfo::GetMaxFeatureLevel(platform) != ERHIFeatureLevel::SM5) throw std::invalid_argument("Shader platform must support SM5");
        for (auto& selected : GRHIGlobals.ShaderPlatformForFeatureLevel) selected = SP_NumPlatforms;
        GRHIGlobals.ShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM5] = platform;
        GMaxRHIFeatureLevel = ERHIFeatureLevel::SM5;
        GMaxRHIShaderPlatform = platform;
    }
}
