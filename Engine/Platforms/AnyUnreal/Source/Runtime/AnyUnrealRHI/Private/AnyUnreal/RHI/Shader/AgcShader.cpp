#include "AnyUnreal/RHI/Shader/AgcShader.h"
#include "AnyUnreal/RHI/Shader/AgcHeader.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/Shader/include/CreateShader.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::size_t align(std::size_t value, std::size_t alignment)
        {
            if (value > std::numeric_limits<std::size_t>::max() - (alignment - 1)) throw std::overflow_error("AGC shader allocation overflow");
            return (value + alignment - 1) & ~(alignment - 1);
        }

        std::size_t allocationSize(const Shader::ShaderBinaryView& binary, Shader::ShaderStage expectedStage, Shader::AgcTarget expectedTarget)
        {
            if (binary.Stage != expectedStage || binary.Target != expectedTarget || expectedTarget > Shader::AgcTarget::Ps5) throw std::invalid_argument("AGC shader stage or platform does not match the runtime request");
            ValidateAgcHeader(binary.Header, binary.Code, binary.Stage);
            const auto offset = align(binary.Header.size(), 256);
            if (binary.Code.size() > std::numeric_limits<std::size_t>::max() - offset) throw std::overflow_error("AGC shader allocation overflow");
            return align(offset + binary.Code.size(), Kernel::DirectAllocation::PageSize);
        }
    }

    AgcShader::AgcShader(const Shader::ShaderBinaryView& binary, Shader::ShaderStage expectedStage, Shader::AgcTarget expectedTarget) : allocation(allocationSize(binary, expectedStage, expectedTarget), Kernel::DirectAllocation::PageSize, Kernel::DirectMemoryAccess::CpuGpu)
    {
        auto* header = static_cast<std::uint8_t*>(allocation.Data());
        auto* code = header + align(binary.Header.size(), 256);
        std::memcpy(header, binary.Header.data(), binary.Header.size());
        std::memcpy(code, binary.Code.data(), binary.Code.size());
        Kernel::CheckResult(sceAgcCreateShader(&shader, header, code), "sceAgcCreateShader");
        if (shader != reinterpret_cast<::Shader*>(header) || shader->code != code) throw std::runtime_error("AGC returned an unexpected shader allocation");
    }

    const ::Shader& AgcShader::Get() const
    {
        return *shader;
    }
}
