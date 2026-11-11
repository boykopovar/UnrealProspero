#include "AnyUnreal/Shader/UserData.h"
#include <charconv>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace AnyUnreal::Shader
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
                if (Get("version") != 21 || Get("target") != 2) throw std::invalid_argument("Unsupported AGC binding metadata version or target");
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
        if (!((stage == ShaderStage::Vertex && sourceStage == 1) || (stage == ShaderStage::Pixel && sourceStage == 5) || (stage == ShaderStage::Compute && sourceStage == 6))) throw std::invalid_argument("AGC binding metadata has the wrong shader stage");
        UserDataLayout result;
        result.count = fields.Get("user_sgpr_count");
        result.addressHigh = fields.Get("address32_hi");
        result.pushConstantSize = fields.Get("push_constant_size");
        if (result.count > 16) throw std::invalid_argument("AGC user data exceeds register capacity");
        if (fields.Flag("scratch_valid") || fields.Flag("streamout_valid")) throw std::invalid_argument("AGC scratch and streamout bindings are not supported");
        for (const auto* name : {"scratch_bytes_per_wave", "scratch_size_per_thread", "scratch_buffer_table_user_data_dword", "streamout_buffer_table_user_data_dword", "streamout_enabled_stream_buffers_mask", "streamout_config_sgpr", "streamout_write_index_sgpr"})
        {
            if (fields.Get(name) != 0) throw std::invalid_argument("Unexpected AGC scratch or streamout state");
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
            const std::uint32_t width = kind == Input::RingTable ? 2 : 1;
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
}
