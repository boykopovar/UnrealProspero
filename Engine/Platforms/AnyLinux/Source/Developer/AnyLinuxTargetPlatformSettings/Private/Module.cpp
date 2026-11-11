#include "AnyUnreal/Cook/SettingsModule.h"
#include "Modules/ModuleManager.h"

using AnyLinuxSettingsModule = AnyUnreal::Cook::SettingsModule<EAnyUnrealTarget::Linux>;
IMPLEMENT_MODULE(AnyLinuxSettingsModule, AnyLinuxTargetPlatformSettings)
