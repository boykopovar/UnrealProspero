#include "AnyUnreal/Shader/PipelineDescriptors.h"
#include "AnyUnreal/Shader/SpirvReflection.h"
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace AnyUnreal::Shader
{
    std::vector<DescriptorBinding> MergePipelineDescriptors(std::span<const PipelineStageDescriptors> stages)
    {
        if (stages.empty()) throw std::invalid_argument("Shader pipeline has no stages");
        struct PreparedStage
        {
            std::vector<std::uint32_t> Words;
            std::vector<DescriptorBinding> Descriptors;
        };
        std::vector<PreparedStage> prepared;
        std::vector<DescriptorBinding> merged;
        std::array<std::uint32_t, 4> nextBinding{};
        std::array<std::uint32_t, 4> nextOffset{};
        prepared.reserve(stages.size());
        for (std::size_t stageIndex = 0; stageIndex < stages.size(); ++stageIndex)
        {
            const auto& stage = stages[stageIndex];
            const auto original = ReflectDescriptorLayout(stage.Words, stage.EntryPoint);
            if (original.size() != stage.Descriptors.size()) throw std::invalid_argument("Pipeline descriptor count disagrees with SPIR-V");
            for (std::size_t previous = 0; previous < stageIndex; ++previous)
            {
                if (&stage.Words == &stages[previous].Words || &stage.Descriptors == &stages[previous].Descriptors) throw std::invalid_argument("Aliased pipeline stage storage");
            }
            auto& destination = prepared.emplace_back(PreparedStage{stage.Words, stage.Descriptors});
            SpirvReflection reflection(stage.Words);
            std::vector<std::pair<const SpvReflectDescriptorBinding*, std::uint32_t>> remaps;
            for (std::size_t index = 0; index < stage.Descriptors.size(); ++index)
            {
                const auto& descriptor = stage.Descriptors[index];
                const auto& expected = original[index];
                if (descriptor.Set != expected.Set || descriptor.Binding != expected.Binding || descriptor.Count != expected.Count || descriptor.Stride != expected.Stride || descriptor.Type != expected.Type || descriptor.Kind != expected.Kind || descriptor.BlockSize != expected.BlockSize) throw std::invalid_argument("Pipeline descriptor type disagrees with SPIR-V");
                if (descriptor.Set >= nextBinding.size() || descriptor.Count == 0 || descriptor.Stride == 0 || descriptor.Stride % 16 != 0) throw std::invalid_argument("Invalid pipeline descriptor layout");
                for (std::size_t previous = 0; previous < index; ++previous)
                {
                    const auto& other = stage.Descriptors[previous];
                    if (descriptor.Set == other.Set && descriptor.Binding == other.Binding) throw std::invalid_argument("Duplicate pipeline stage binding");
                }
                SpvReflectResult status;
                auto* reflected = spvReflectGetDescriptorBinding(reflection.Get(), descriptor.Binding, descriptor.Set, &status);
                SpirvReflection::Check(status);
                if (!reflected || reflected->count != descriptor.Count) throw std::invalid_argument("Pipeline descriptors disagree with SPIR-V");
                auto& binding = nextBinding[descriptor.Set];
                auto& offset = nextOffset[descriptor.Set];
                const auto size = static_cast<std::uint64_t>(descriptor.Count) * descriptor.Stride;
                if (binding == std::numeric_limits<std::uint32_t>::max() || size > std::numeric_limits<std::uint32_t>::max() - offset) throw std::overflow_error("Pipeline descriptor table exceeds format limits");
                auto& updated = destination.Descriptors[index];
                updated.Binding = binding++;
                updated.Offset = offset;
                offset += static_cast<std::uint32_t>(size);
                remaps.emplace_back(reflected, updated.Binding);
                merged.push_back(updated);
            }
            for (const auto& [descriptor, binding] : remaps) reflection.RemapBinding(descriptor, binding);
            const auto* module = reflection.Get();
            const auto size = spvReflectGetCodeSize(module);
            if (size % sizeof(std::uint32_t) != 0) throw std::runtime_error("Invalid remapped SPIR-V size");
            const auto* words = spvReflectGetCode(module);
            destination.Words.assign(words, words + size / sizeof(std::uint32_t));
        }
        for (std::size_t index = 0; index < stages.size(); ++index)
        {
            stages[index].Words.swap(prepared[index].Words);
            stages[index].Descriptors.swap(prepared[index].Descriptors);
        }
        return merged;
    }
}
