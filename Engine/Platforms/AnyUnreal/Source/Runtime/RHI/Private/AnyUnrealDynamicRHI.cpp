#include "DynamicRHI.h"
#include "Modules/ModuleManager.h"
#include <stdexcept>

const TCHAR* GetSelectedDynamicRHIModuleName(bool)
{
    return TEXT("AnyUnrealRHI");
}

FDynamicRHI* PlatformCreateDynamicRHI()
{
    auto& module = FModuleManager::LoadModuleChecked<IDynamicRHIModule>(GetSelectedDynamicRHIModuleName());
    if (!module.IsSupported(ERHIFeatureLevel::SM5)) throw std::runtime_error("AnyUnreal RHI is unavailable");
    auto* device = module.CreateRHI(ERHIFeatureLevel::SM5);
    if (!device) throw std::runtime_error("AnyUnreal RHI did not create a device");
    return device;
}
