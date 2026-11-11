#include "UnrealProspero/Cook/SettingsModule.h"
#include "Modules/ModuleManager.h"

using AnyPS5SettingsModule = UnrealProspero::Cook::SettingsModule<EUnrealProsperoTarget::Ps5>;
IMPLEMENT_MODULE(AnyPS5SettingsModule, AnyPS5TargetPlatformSettings)
