#include "UnrealProspero/Cook/SettingsModule.h"
#include "Modules/ModuleManager.h"

using AnyLinuxSettingsModule = UnrealProspero::Cook::SettingsModule<EUnrealProsperoTarget::Linux>;
IMPLEMENT_MODULE(AnyLinuxSettingsModule, AnyLinuxTargetPlatformSettings)
