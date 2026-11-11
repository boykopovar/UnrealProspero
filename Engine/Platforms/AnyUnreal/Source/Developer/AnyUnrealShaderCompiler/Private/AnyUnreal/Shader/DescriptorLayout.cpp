#include "AnyUnreal/Shader/DescriptorLayout.h"
#include "spirv_reflect.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace AnyUnreal::Shader
{
    namespace
    {
        void check(SpvReflectResult result)
        {
            if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("SPIR-V reflection failed: " + std::to_string(result));
        }

        class Reflection final
        {
        public:
            explicit Reflection(std::span<const std::uint32_t> spirv)
            {
                check(spvReflectCreateShaderModule(spirv.size_bytes(), spirv.data(), &module));
            }

            ~Reflection()
            {
                spvReflectDestroyShaderModule(&module);
            }

            Reflection(const Reflection&) = delete;
            Reflection& operator=(const Reflection&) = delete;
            const SpvReflectShaderModule* Get() const { return &module; }

        private:
            SpvReflectShaderModule module{};
        };

        std::pair<const char*, std::uint32_t> describe(SpvReflectDescriptorType type)
        {
            switch (type)
            {
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return {"sampler", 16};
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return {"combined-image-sampler", 48};
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return {"sampled-image", 32};
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return {"storage-image", 32};
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return {"uniform-texel-buffer", 16};
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return {"storage-texel-buffer", 16};
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return {"uniform-buffer", 16};
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return {"storage-buffer", 16};
                case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return {"input-attachment", 32};
                default: throw std::invalid_argument("Unsupported AGC descriptor type: " + std::to_string(type));
            }
        }
    }

    std::vector<DescriptorBinding> ReflectDescriptorLayout(std::span<const std::uint32_t> spirv, const std::string& entryPoint)
    {
        if (spirv.size() < 5 || spirv.front() != 0x07230203) throw std::invalid_argument("Invalid SPIR-V header");
        if (entryPoint.empty() || entryPoint.find('\0') != std::string::npos) throw std::invalid_argument("Invalid shader entry point");
        Reflection reflection(spirv);
        if (!spvReflectGetEntryPoint(reflection.Get(), entryPoint.c_str())) throw std::invalid_argument("Shader entry point does not exist");
        std::uint32_t count = 0;
        check(spvReflectEnumerateEntryPointDescriptorBindings(reflection.Get(), entryPoint.c_str(), &count, nullptr));
        std::vector<SpvReflectDescriptorBinding*> bindings(count);
        if (count != 0) check(spvReflectEnumerateEntryPointDescriptorBindings(reflection.Get(), entryPoint.c_str(), &count, bindings.data()));
        std::sort(bindings.begin(), bindings.end(), [](const auto* left, const auto* right)
        {
            return std::tie(left->set, left->binding) < std::tie(right->set, right->binding);
        });
        std::vector<DescriptorBinding> result;
        result.reserve(bindings.size());
        std::uint64_t offset = 0;
        for (const auto* binding : bindings)
        {
            std::uint64_t arrayCount = 1;
            for (std::uint32_t dimension = 0; dimension < binding->array.dims_count; ++dimension)
            {
                const auto size = binding->array.dims[dimension];
                if (size == 0) throw std::invalid_argument("Runtime descriptor arrays require an explicit capacity");
                arrayCount *= size;
                if (arrayCount > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Shader descriptor array is too large");
            }
            if (arrayCount != binding->count) throw std::runtime_error("Inconsistent shader descriptor array size");
            if (!result.empty() && result.back().Set == binding->set && result.back().Binding == binding->binding) throw std::invalid_argument("Duplicate shader descriptor binding");
            if (result.empty() || result.back().Set != binding->set) offset = 0;
            const auto [type, stride] = describe(binding->descriptor_type);
            const std::uint64_t end = offset + static_cast<std::uint64_t>(binding->count) * stride;
            if (end > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Shader descriptor table is too large");
            result.push_back({binding->set, binding->binding, binding->count, static_cast<std::uint32_t>(offset), stride, type, binding->name ? binding->name : ""});
            offset = end;
        }
        return result;
    }
}
