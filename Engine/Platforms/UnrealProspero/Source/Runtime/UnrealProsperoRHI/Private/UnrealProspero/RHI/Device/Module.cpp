#include "UnrealProspero/RHI/Device/Module.h"
#include "UnrealProspero/RHI/Device/DynamicRHI.h"
#include <stdexcept>
#include <type_traits>

namespace UnrealProspero::RHI
{
    static_assert(!std::is_abstract_v<DynamicRHI>);

    bool Module::IsSupported() { return true; }

    bool Module::IsSupported(ERHIFeatureLevel::Type requestedFeatureLevel)
    {
        return requestedFeatureLevel == ERHIFeatureLevel::SM5 || requestedFeatureLevel == ERHIFeatureLevel::Num;
    }

    FDynamicRHI* Module::CreateRHI(ERHIFeatureLevel::Type requestedFeatureLevel)
    {
        if (!IsSupported(requestedFeatureLevel)) throw std::invalid_argument("UnrealProspero requires the SM5 rendering path");
#if UNREALPROSPERO_TARGET_WINDOWS
        constexpr auto target = Shader::AgcTarget::Windows;
#elif UNREALPROSPERO_TARGET_LINUX
        constexpr auto target = Shader::AgcTarget::Linux;
#elif UNREALPROSPERO_TARGET_PS5
        constexpr auto target = Shader::AgcTarget::Ps5;
#else
#error UnrealProspero requires a target platform
#endif
        return new DynamicRHI(target, 2);
    }
}
