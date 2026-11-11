#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/File/Path.h"
#include "UnrealProspero/Kernel/Module.h"
#include "UnrealProspero/Memory/SystemAllocator.h"
#include "UnrealProspero/Text/Encoding.h"
#include <memory>
#include <stdexcept>

void* FUnrealProsperoPlatformProcess::GetDllHandle(const TCHAR* filename)
{
    const auto path = UnrealProspero::File::EncodePath(filename);
    UnrealProspero::Memory::SystemAllocator<UnrealProspero::Kernel::Module> allocator;
    auto* module = allocator.allocate(1);
    try
    {
        std::construct_at(module, path);
    }
    catch (...)
    {
        allocator.deallocate(module, 1);
        throw;
    }
    return module;
}

void FUnrealProsperoPlatformProcess::FreeDllHandle(void* handle)
{
    if (handle == nullptr) throw std::invalid_argument("Module handle is null");
    auto* module = static_cast<UnrealProspero::Kernel::Module*>(handle);
    module->Close();
    std::destroy_at(module);
    UnrealProspero::Memory::SystemAllocator<UnrealProspero::Kernel::Module>{}.deallocate(module, 1);
}

void* FUnrealProsperoPlatformProcess::GetDllExport(void* handle, const TCHAR* name)
{
    if (handle == nullptr) throw std::invalid_argument("Module handle is null");
    if (name == nullptr) throw std::invalid_argument("Module symbol name is null");
    return static_cast<UnrealProspero::Kernel::Module*>(handle)->FindSymbol(UnrealProspero::Text::EncodeUtf8(name));
}
