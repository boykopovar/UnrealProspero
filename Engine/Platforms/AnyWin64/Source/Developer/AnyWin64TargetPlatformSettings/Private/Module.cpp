#include "AnyUnreal/Cook/SettingsModule.h"
#include "Modules/ModuleManager.h"

using AnyWin64SettingsModule = AnyUnreal::Cook::SettingsModule<EAnyUnrealTarget::Windows>;
IMPLEMENT_MODULE(AnyWin64SettingsModule, AnyWin64TargetPlatformSettings)
