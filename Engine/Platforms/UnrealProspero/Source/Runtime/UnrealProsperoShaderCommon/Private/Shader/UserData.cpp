#include "UnrealProspero/Shader/UserData.h"
#include <charconv>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace UnrealProspero::Shader
{
    namespace
    {
        class Fields final
        {
        public:
            explicit Fields(std::span<const std::uint8_t> bytes)
            {
                if (bytes.empty()) throw std::invalid_argument("Missing AGC binding metadata");
                std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                bool first = true;
                while (!text.empty())
                {
                    const auto end = text.find('\n');
                    if (end == std::string_view::npos) throw std::invalid_argument("Truncated AGC binding metadata");
                    auto line = text.substr(0, end);
                    text.remove_prefix(end + 1);
                    if (line.ends_with('\r')) line.remove_suffix(1);
                    if (first)
                    {
                        if (line != "adk-psbc-metadata-v1") throw std::invalid_argument("Unsupported AGC metadata encoding");
                        first = false;
                        continue;
                    }
                    const auto split = line.find('=');
                    if (split == 0 || split == std::string_view::npos) throw std::invalid_argument("Invalid AGC metadata field");
                    const auto name = line.substr(0, split);
                    if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_[].") != std::string_view::npos) throw std::invalid_argument("Invalid AGC metadata field name");
                    const auto number = line.substr(split + 1);
                    std::uint64_t value = 0;
                    const auto parsed = std::from_chars(number.data(), number.data() + number.size(), value);
                    if (parsed.ec != std::errc{} || parsed.ptr != number.data() + number.size()) throw std::invalid_argument("Invalid AGC metadata number");
                    if (!values.emplace(std::string(name), value).second) throw std::invalid_argument("Duplicate AGC metadata field");
                }
                if (Get("version") != 22 || Get("target") != 2) throw std::invalid_argument("Unsupported AGC binding metadata version or target");
            }

            std::uint32_t Get(const std::string& name) const
            {
                const auto found = values.find(name);
                if (found == values.end() || found->second > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Missing or out-of-range AGC metadata field: " + name);
                return static_cast<std::uint32_t>(found->second);
            }

            bool Flag(const std::string& name) const
            {
                const auto value = Get(name);
                if (value > 1) throw std::invalid_argument("Invalid AGC metadata flag: " + name);
                return value != 0;
            }

        private:
            std::map<std::string, std::uint64_t> values;
        };
    }

    UserDataLayout UserDataLayout::Read(std::span<const std::uint8_t> metadata, ShaderStage stage)
    {
        const Fields fields(metadata);
        const auto sourceStage = fields.Get("source_stage");
        if (!((stage == ShaderStage::Vertex && sourceStage == 1) || (stage == ShaderStage::Pixel && sourceStage == 5) || (stage == ShaderStage::Compute && sourceStage == 6) || (stage == ShaderStage::Geometry && sourceStage == 4))) throw std::invalid_argument("AGC binding metadata has the wrong shader stage");
        const bool vertexInput = stage == ShaderStage::Vertex || stage == ShaderStage::Geometry;
        if (stage == ShaderStage::Geometry && (!fields.Flag("merged_geometry") || fields.Get("merged_es_source_stage") != 1 || !fields.Flag("esgs_system_sgprs_valid") || fields.Get("merged_es_itemsize") == 0 || fields.Get("merged_esgs_ring_itemsize") == 0)) throw std::invalid_argument("Geometry program does not contain its vertex export stage");
        UserDataLayout result;
        result.stage = stage;
        result.count = fields.Get("user_sgpr_count");
        result.addressHigh = fields.Get("address32_hi");
        result.pushConstantSize = fields.Get("push_constant_size");
        result.vertexBufferUsageMask = fields.Get("vertex_buffer_usage_mask");
        result.vertexBuffersPerAttribute = fields.Flag("vertex_buffer_per_attribute");
        result.dynamicVertexInput = fields.Flag("dynamic_vertex_input");
        const auto hasVertexBuffers = fields.Flag("vertex_buffer_table_valid");
        if (hasVertexBuffers != (result.vertexBufferUsageMask != 0) || (!hasVertexBuffers && result.vertexBuffersPerAttribute) || (hasVertexBuffers && !vertexInput)) throw std::invalid_argument("Invalid AGC vertex buffer table metadata");
        if (result.dynamicVertexInput && (!result.vertexBuffersPerAttribute || !vertexInput)) throw std::invalid_argument("Invalid dynamic vertex input metadata");
        const auto descriptorStride = hasVertexBuffers ? (result.dynamicVertexInput ? 32u : 16u) : 0u;
        if (fields.Get("vertex_buffer_descriptor_stride") != descriptorStride) throw std::invalid_argument("Invalid vertex buffer descriptor stride");
        if (result.count > 16) throw std::invalid_argument("AGC user data exceeds register capacity");
        const auto hasScratch = fields.Flag("scratch_valid");
        result.scratchBytesPerWave = fields.Get("scratch_bytes_per_wave");
        const auto scratchSizePerThread = fields.Get("scratch_size_per_thread");
        if (hasScratch)
        {
            if (stage != ShaderStage::Compute || result.scratchBytesPerWave == 0 || result.scratchBytesPerWave % 128 || scratchSizePerThread > result.scratchBytesPerWave / 32) throw std::invalid_argument("Invalid AGC compute scratch extent");
        }
        else if (result.scratchBytesPerWave || scratchSizePerThread) throw std::invalid_argument("Unexpected AGC scratch extent");
        if (fields.Flag("streamout_valid")) throw std::invalid_argument("AGC streamout bindings are not supported");
        for (const auto* name : {"streamout_buffer_table_user_data_dword", "streamout_enabled_stream_buffers_mask", "streamout_config_sgpr", "streamout_write_index_sgpr"})
        {
            if (fields.Get(name) != 0) throw std::invalid_argument("Unexpected AGC streamout state");
        }
        for (unsigned index = 0; index < 4; ++index)
        {
            const auto suffix = "[" + std::to_string(index) + "]";
            if (fields.Get("streamout_strides_dwords" + suffix) != 0 || fields.Get("streamout_offset_sgprs" + suffix) != 0) throw std::invalid_argument("Unexpected AGC streamout buffer state");
        }
        std::array<bool, 16> used{};
        const auto add = [&](Input kind, bool valid, std::uint32_t slot, std::uint32_t value)
        {
            if (!valid)
            {
                if (slot != 0) throw std::invalid_argument("AGC slot supplied for an absent resource");
                return;
            }
            const std::uint32_t width = kind == Input::RingTable || kind == Input::ScratchAddress ? 2 : 1;
            if (slot >= result.count || width > result.count - slot) throw std::invalid_argument("AGC user data slot exceeds its register window");
            for (std::uint32_t index = slot; index < slot + width; ++index)
            {
                if (used[index]) throw std::invalid_argument("Overlapping AGC user data slots");
                used[index] = true;
            }
            result.bindings.push_back({kind, slot, value});
        };
        const auto field = [&](Input kind, const std::string& name, std::uint32_t value = 0)
        {
            add(kind, fields.Flag(name + "_valid"), fields.Get(name + "_user_data_dword"), value);
        };
        field(Input::VertexBuffers, "vertex_buffer_table");
        field(Input::PushConstants, "push_constants");
        field(Input::BaseVertex, "base_vertex");
        field(Input::StartInstance, "start_instance");
        field(Input::DrawId, "draw_id");
        field(Input::ViewIndex, "view_index");
        field(Input::RingTable, "ps5_ring_table");
        add(Input::ScratchAddress, hasScratch, fields.Get("scratch_buffer_table_user_data_dword"), 0);
        field(Input::Constant, "ngg_lds_layout", fields.Get("ngg_lds_layout"));
        for (std::uint32_t index = 0; index < 4; ++index)
        {
            const auto suffix = "[" + std::to_string(index) + "]";
            add(Input::DescriptorSet, fields.Flag("descriptor_set_valid" + suffix), fields.Get("descriptor_set_user_data_dword" + suffix), index);
        }
        if (fields.Flag("descriptor_set0_valid") != fields.Flag("descriptor_set_valid[0]") || fields.Get("descriptor_set0_user_data_dword") != fields.Get("descriptor_set_user_data_dword[0]")) throw std::invalid_argument("AGC descriptor set zero aliases disagree");
        if (fields.Flag("push_constants_valid") != (result.pushConstantSize != 0) || result.pushConstantSize > 256) throw std::invalid_argument("Invalid AGC push constant extent");
        return result;
    }

    std::array<std::uint32_t, 16> UserDataLayout::Build(const UserDataInputs& inputs) const
    {
        std::array<std::uint32_t, 16> result{};
        for (const auto& binding : bindings)
        {
            std::uint64_t address = 0;
            switch (binding.Kind)
            {
                case Input::Constant: result[binding.Slot] = binding.Value; continue;
                case Input::BaseVertex: result[binding.Slot] = inputs.BaseVertex; continue;
                case Input::StartInstance: result[binding.Slot] = inputs.StartInstance; continue;
                case Input::DrawId: result[binding.Slot] = inputs.DrawId; continue;
                case Input::ViewIndex: result[binding.Slot] = inputs.ViewIndex; continue;
                case Input::DescriptorSet: address = inputs.DescriptorSets[binding.Value]; break;
                case Input::VertexBuffers: address = inputs.VertexBuffers; break;
                case Input::PushConstants: address = inputs.PushConstants; break;
                case Input::RingTable: address = inputs.RingTable; break;
                case Input::ScratchAddress:
                    address = inputs.ScratchAddress;
                    if (address == 0 || address % 256 != 0 || address >= (std::uint64_t{1} << 48)) throw std::invalid_argument("Invalid AGC compute scratch address");
                    result[binding.Slot] = static_cast<std::uint32_t>(address);
                    result[binding.Slot + 1] = static_cast<std::uint32_t>(address >> 32) | 0x80000000u;
                    continue;
            }
            const auto alignment = binding.Kind == Input::PushConstants ? 4u : 16u;
            if (address == 0 || address % alignment != 0) throw std::invalid_argument("Missing or unaligned AGC resource address");
            if (binding.Kind == Input::RingTable) result[binding.Slot + 1] = static_cast<std::uint32_t>(address >> 32);
            else if ((address >> 32) != addressHigh) throw std::invalid_argument("AGC resource address is outside the shader aperture");
            result[binding.Slot] = static_cast<std::uint32_t>(address);
        }
        return result;
    }

    std::uint32_t UserDataLayout::GetCount() const { return count; }
    std::uint32_t UserDataLayout::GetAddressHigh() const { return addressHigh; }
    std::uint32_t UserDataLayout::GetPushConstantSize() const { return pushConstantSize; }
    std::uint32_t UserDataLayout::GetScratchBytesPerWave() const { return scratchBytesPerWave; }
    ShaderStage UserDataLayout::GetStage() const { return stage; }
    std::uint32_t UserDataLayout::GetVertexBufferUsageMask() const { return vertexBufferUsageMask; }
    bool UserDataLayout::UsesVertexBuffersPerAttribute() const { return vertexBuffersPerAttribute; }
    bool UserDataLayout::UsesDynamicVertexInput() const { return dynamicVertexInput; }

    std::optional<std::uint32_t> UserDataLayout::GetBaseVertexSlot() const
    {
        for (const auto& binding : bindings) if (binding.Kind == Input::BaseVertex) return binding.Slot;
        return std::nullopt;
    }

    std::optional<std::uint32_t> UserDataLayout::GetStartInstanceSlot() const
    {
        for (const auto& binding : bindings) if (binding.Kind == Input::StartInstance) return binding.Slot;
        return std::nullopt;
    }
}
