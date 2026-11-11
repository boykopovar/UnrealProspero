#include "AnyUnreal/Cook/SettingsModule.h"
#include "Modules/ModuleManager.h"

using AnyPS5SettingsModule = AnyUnreal::Cook::SettingsModule<EAnyUnrealTarget::Ps5>;
IMPLEMENT_MODULE(AnyPS5SettingsModule, AnyPS5TargetPlatformSettings)
