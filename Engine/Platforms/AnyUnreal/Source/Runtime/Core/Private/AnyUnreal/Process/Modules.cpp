#include "AnyUnreal/AnyUnrealPlatformProcess.h"
#include "AnyUnreal/File/Path.h"
#include "AnyUnreal/Kernel/Module.h"
#include "AnyUnreal/Memory/SystemAllocator.h"
#include "AnyUnreal/Text/Encoding.h"
#include <memory>
#include <stdexcept>

void* FAnyUnrealPlatformProcess::GetDllHandle(const TCHAR* filename)
{
    const auto path = AnyUnreal::File::EncodePath(filename);
    AnyUnreal::Memory::SystemAllocator<AnyUnreal::Kernel::Module> allocator;
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

void FAnyUnrealPlatformProcess::FreeDllHandle(void* handle)
{
    if (handle == nullptr) throw std::invalid_argument("Module handle is null");
    auto* module = static_cast<AnyUnreal::Kernel::Module*>(handle);
    module->Close();
    std::destroy_at(module);
    AnyUnreal::Memory::SystemAllocator<AnyUnreal::Kernel::Module>{}.deallocate(module, 1);
}

void* FAnyUnrealPlatformProcess::GetDllExport(void* handle, const TCHAR* name)
{
    if (handle == nullptr) throw std::invalid_argument("Module handle is null");
    if (name == nullptr) throw std::invalid_argument("Module symbol name is null");
    return static_cast<AnyUnreal::Kernel::Module*>(handle)->FindSymbol(AnyUnreal::Text::EncodeUtf8(name));
}
