#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "AnyUnreal/RHI/Texture/DepthTexture2D.h"
#include <iostream>
#include <memory>
#include <stdexcept>

int main()
{
    using namespace AnyUnreal::RHI;
    const auto baseline = BufferAllocation::GetTextureAllocationBytes();
    BufferAllocation buffer(32769, 1);
    if (BufferAllocation::GetTextureAllocationBytes() != baseline) throw std::runtime_error("Buffer allocation was counted as a texture");
    std::shared_ptr<const BufferAllocation> retained;
    std::uint64_t retainedBytes = 0;
    {
        auto texture = std::make_shared<LinearTexture2D>(137, 71, TextureFormat::Rgba16Float, 1);
        retainedBytes = texture->GetStorage().AllocationSize();
        if (BufferAllocation::GetTextureAllocationBytes() != baseline + retainedBytes) throw std::runtime_error("Color texture allocation was not counted");
        retained = {texture, &texture->GetStorage()};
        {
            DepthTexture2D depth(129, 65, 1);
            if (BufferAllocation::GetTextureAllocationBytes() != baseline + retainedBytes + depth.GetStorage().AllocationSize()) throw std::runtime_error("Depth texture allocation was not counted");
        }
        if (BufferAllocation::GetTextureAllocationBytes() != baseline + retainedBytes) throw std::runtime_error("Released depth texture remained counted");
    }
    if (BufferAllocation::GetTextureAllocationBytes() != baseline + retainedBytes) throw std::runtime_error("Retained texture storage was removed from the statistics");
    retained.reset();
    if (BufferAllocation::GetTextureAllocationBytes() != baseline) throw std::runtime_error("Released texture storage remained counted");
    bool rejected = false;
    try { LinearTexture2D invalid(0, 1, TextureFormat::Rgba8Unorm, 1); }
    catch (const std::out_of_range&) { rejected = true; }
    if (!rejected || BufferAllocation::GetTextureAllocationBytes() != baseline) throw std::runtime_error("Failed texture allocation changed memory statistics");
    std::cout << "Texture memory accounting verified across allocation, retention and release\n";
    return 0;
}
