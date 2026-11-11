#include "UnrealProspero/RHI/Descriptor/MergedTables.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        DescriptorStorage mergeTables(std::span<const DescriptorTables* const> sources, std::uint32_t addressHigh)
        {
            if (sources.empty()) throw std::invalid_argument("No descriptor tables to merge");
            std::vector<DescriptorRange> ranges;
            std::array<std::size_t, 4> sizes{};
            for (std::size_t index = 0; index < sources.size(); ++index)
            {
                const auto* source = sources[index];
                if (!source) throw std::invalid_argument("Null descriptor table");
                if (std::find(sources.begin(), sources.begin() + index, source) != sources.begin() + index) throw std::invalid_argument("Duplicate descriptor table");
                for (const auto& range : source->GetRanges())
                {
                    const auto end = static_cast<std::uint64_t>(range.Offset) + range.Bytes.size();
                    if (range.Set >= sizes.size() || range.Bytes.empty() || end > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Invalid descriptor range");
                    for (const auto& existing : ranges)
                    {
                        if (range.Set != existing.Set) continue;
                        if (range.Binding == existing.Binding || (range.Offset < existing.Offset + existing.Bytes.size() && existing.Offset < end)) throw std::invalid_argument("Pipeline descriptor ranges overlap");
                    }
                    sizes[range.Set] = std::max(sizes[range.Set], static_cast<std::size_t>(end));
                    ranges.push_back(range);
                }
            }
            DescriptorStorage result(sizes, addressHigh);
            for (const auto& range : ranges) result.Upload(range.Set, range.Offset, range.Bytes);
            return result;
        }
    }

    MergedDescriptorTables::MergedDescriptorTables(std::span<const DescriptorTables* const> sources, std::uint32_t addressHigh) : storage(mergeTables(sources, addressHigh)) {}
    void MergedDescriptorTables::Apply(Shader::UserDataInputs& inputs) const { storage.Apply(inputs); }
    std::vector<GpuMemoryRange> MergedDescriptorTables::GetMemoryRanges() const { return storage.GetMemoryRanges(); }
}
