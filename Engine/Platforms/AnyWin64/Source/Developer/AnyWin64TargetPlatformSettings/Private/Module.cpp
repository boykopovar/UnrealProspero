#include "UnrealProspero/Cook/SettingsModule.h"
#include "Modules/ModuleManager.h"

using AnyWin64SettingsModule = UnrealProspero::Cook::SettingsModule<EUnrealProsperoTarget::Windows>;
IMPLEMENT_MODULE(AnyWin64SettingsModule, AnyWin64TargetPlatformSettings)
