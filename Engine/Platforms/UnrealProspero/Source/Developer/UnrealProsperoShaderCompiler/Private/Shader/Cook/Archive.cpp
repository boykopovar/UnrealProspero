#include "UnrealProspero/Shader/Cook/Archive.h"
#include "UnrealProspero/Shader/ShaderBinary.h"
#include "HAL/FileManager.h"
#include "Misc/Compression.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include <limits>
#include <exception>
#include <memory>
#include <set>
#include <stdexcept>

namespace UnrealProspero::Shader::Cook
{
    namespace
    {
        void validate(const FSerializedShaderArchive& archive)
        {
            if (archive.GetShaderHashes().Num() != archive.GetShaderEntries().Num() || archive.GetShaderMapHashes().Num() != archive.GetShaderMapEntries().Num()) throw std::invalid_argument("Shader archive hashes disagree with its index");
            const auto indices = archive.GetShaderIndices();
            for (const auto shader : indices)
            {
                if (shader >= static_cast<std::uint32_t>(archive.GetShaderEntries().Num())) throw std::invalid_argument("Shader map refers to a missing shader");
            }
            for (const auto& map : archive.GetShaderMapEntries())
            {
                if (map.NumShaders == 0 || static_cast<std::uint64_t>(map.ShaderIndicesOffset) + map.NumShaders > static_cast<std::uint64_t>(indices.Num())) throw std::invalid_argument("Invalid shader map index range");
                std::set<std::uint32_t> seen;
                for (std::uint32_t offset = 0; offset < map.NumShaders; ++offset)
                {
                    if (!seen.insert(indices[map.ShaderIndicesOffset + offset]).second) throw std::invalid_argument("Shader map contains a duplicate shader index");
                }
            }
        }
    }

    Archive::Archive(const FString& path)
    {
        std::unique_ptr<FArchive> reader(IFileManager::Get().CreateFileReader(*path));
        if (!reader || !FSerializedShaderArchive::SerializeHeaderVersion(*reader)) throw std::invalid_argument("Missing or unsupported shader archive");
        index.Serialize(*reader);
        if (reader->IsError()) throw std::invalid_argument("Truncated shader archive index");
        validate(index);
        const auto base = reader->Tell();
        const auto total = reader->TotalSize();
        if (base < 0 || total < base) throw std::invalid_argument("Invalid shader archive payload offset");
        std::uint64_t offset = 0;
        code.Reserve(index.GetShaderEntries().Num());
        for (const auto& entry : index.GetShaderEntries())
        {
            if (entry.IsPartOfLibrary() || entry.GetFrequency() >= SF_NumFrequencies || entry.Offset != offset || entry.Size == 0 || entry.Size > static_cast<std::uint32_t>(std::numeric_limits<int32>::max()) || entry.GetUncompressedSize() == 0 || entry.GetUncompressedSize() > static_cast<std::uint32_t>(std::numeric_limits<int32>::max())) throw std::invalid_argument("Invalid shader archive code entry");
            if (entry.Offset > static_cast<std::uint64_t>(total - base) || entry.Size > static_cast<std::uint64_t>(total - base) - entry.Offset) throw std::invalid_argument("Truncated shader archive payload");
            auto buffer = FUniqueBuffer::Alloc(entry.Size);
            reader->Serialize(buffer.GetData(), entry.Size);
            if (reader->IsError()) throw std::runtime_error("Cannot read shader archive code");
            code.Add(buffer.MoveToShared());
            offset += entry.Size;
        }
        if (offset != static_cast<std::uint64_t>(total - base)) throw std::invalid_argument("Trailing shader archive data");
        if (!reader->Close()) throw std::runtime_error("Cannot close shader archive reader");
    }

    const FSerializedShaderArchive& Archive::GetIndex() const { return index; }

    TArray<uint8> Archive::GetCode(int32 shaderIndex) const
    {
        if (!index.ShaderEntries.IsValidIndex(shaderIndex)) throw std::out_of_range("Shader archive index is out of range");
        const auto& entry = index.ShaderEntries[shaderIndex];
        const auto& source = code[shaderIndex];
        if (source.GetSize() != entry.Size) throw std::runtime_error("Shader archive code size disagrees with its index");
        TArray<uint8> result;
        result.SetNumUninitialized(static_cast<int32>(entry.GetUncompressedSize()));
        if (entry.IsCompressed())
        {
            if (!FCompression::UncompressMemory(NAME_Oodle, result.GetData(), result.Num(), source.GetData(), source.GetSize())) throw std::runtime_error("Cannot decompress shader archive code");
        }
        else
        {
            if (source.GetSize() != static_cast<std::uint64_t>(result.Num())) throw std::invalid_argument("Invalid uncompressed shader entry size");
            FMemory::Memcpy(result.GetData(), source.GetData(), source.GetSize());
        }
        DecodeUnrealShaderEnvelope(std::span(result.GetData(), static_cast<std::size_t>(result.Num())));
        return result;
    }

    void Archive::SetCode(int32 shaderIndex, std::span<const std::uint8_t> bytes)
    {
        if (!index.ShaderEntries.IsValidIndex(shaderIndex)) throw std::out_of_range("Shader archive index is out of range");
        DecodeUnrealShaderEnvelope(bytes);
        auto& entry = index.ShaderEntries[shaderIndex];
        const auto buffer = FSharedBuffer::Clone(FMemoryView(bytes.data(), bytes.size()));
        entry.Size = static_cast<std::uint32_t>(bytes.size());
        entry.SetUncompressedSize(entry.Size);
        code[shaderIndex] = buffer;
    }

    void Archive::Save(const FString& path) const
    {
        validate(index);
        auto finalized = index;
        for (auto& map : finalized.ShaderMapEntries) map.NumPreloadEntries = 0;
        finalized.Finalize();
        if (!IFileManager::Get().DirectoryExists(*FPaths::GetPath(path))) throw std::invalid_argument("Shader archive output directory does not exist");
        const auto temporary = path + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".tmp");
        std::unique_ptr<FArchive> writer(IFileManager::Get().CreateFileWriter(*temporary));
        if (!writer) throw std::runtime_error("Cannot create shader archive output");
        try
        {
            if (!FSerializedShaderArchive::SerializeHeaderVersion(*writer)) throw std::runtime_error("Cannot serialize shader archive version");
            finalized.Serialize(*writer);
            for (const auto& buffer : code) writer->Serialize(const_cast<void*>(buffer.GetData()), buffer.GetSize());
            if (writer->IsError() || !writer->Close()) throw std::runtime_error("Cannot write shader archive output");
            writer.reset();
            if (!IFileManager::Get().Move(*path, *temporary, true, true, false, true)) throw std::runtime_error("Cannot replace shader archive output");
        }
        catch (...)
        {
            writer.reset();
            if (IFileManager::Get().FileExists(*temporary) && !IFileManager::Get().Delete(*temporary, true, true)) std::throw_with_nested(std::runtime_error("Cannot remove failed shader archive output"));
            throw;
        }
    }
}
