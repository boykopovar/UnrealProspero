import argparse
import re
from pathlib import Path


def apply(engine):
    runtime = engine / "Source/Runtime"
    ticker_sites = {
        "Dataflow/Core/Private/Dataflow/DataflowContextEvaluator.cpp": 2,
        "Dataflow/Core/Private/Dataflow/DataflowTaskGraph.cpp": 1,
        "Online/BuildPatchServices/Private/BuildPatchGeneration.cpp": 1,
        "Online/BuildPatchServices/Private/Generation/ChunkHarvester.cpp": 1,
        "Online/BuildPatchServices/Private/Generation/ChunkDeltaOptimiser.cpp": 1,
        "Online/Experimental/BandwidthMeasurementTool/Private/BandwidthMeasurementTool.cpp": 2,
        "Online/Experimental/DownlinkBandwidthManager/Private/DownlinkBandwidthManager.cpp": 1,
    }
    old_ticker = b"FTSTicker::GetCoreTicker().RemoveTicker("
    new_ticker = b"FTSTicker::RemoveTicker("
    changes = {}
    for name, count in ticker_sites.items():
        path = runtime / name
        data = path.read_bytes()
        if data.count(old_ticker) == count:
            changes[path] = data.replace(old_ticker, new_ticker)
        elif old_ticker in data or data.count(new_ticker) < count:
            raise RuntimeError(f"Unexpected ticker calls in {path}")

    path = runtime / "Online/Experimental/DownlinkBandwidthManager/Private/DownlinkBandwidthManager.cpp"
    data = changes.get(path, path.read_bytes())
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    old_seed = newline.join((b"\tstd::random_device Device;", b"\tstd::mt19937 Generator(Device());"))
    new_seed = newline.join((
        b"\tconst FGuid RandomSeed = FGuid::NewGuid();",
        b"\tstd::seed_seq Seed{RandomSeed.A, RandomSeed.B, RandomSeed.C, RandomSeed.D};",
        b"\tstd::mt19937 Generator(Seed);",
    ))
    guid_header = b'#include "Misc/Guid.h"'
    if data.count(old_seed) == 1 and data.count(b"#include <random>") == 1:
        data = data.replace(old_seed, new_seed)
        if guid_header not in data:
            data = data.replace(b"#include <random>", guid_header + newline + b"#include <random>")
        changes[path] = data
    elif data.count(new_seed) != 1 or guid_header not in data or old_seed in data:
        raise RuntimeError(f"Unexpected random seed code in {path}")

    path = runtime / "Experimental/Chaos/Private/Chaos/PBDSuspensionConstraints.cpp"
    data = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in data else b"\n"
    old_solver = newline.join((
        b"\t\tCollisionSolvers.Add(Private::FPBDCollisionSolver());",
        b"\t\tCollisionSolvers[CollisionSolvers.Num() - 1].MakeInitialized();",
    ))
    new_solver = b"\t\tCollisionSolvers.Add(Private::FPBDCollisionSolver::MakeInitialized());"
    if data.count(old_solver) == 1 and new_solver not in data:
        changes[path] = data.replace(old_solver, new_solver)
    elif old_solver in data or data.count(new_solver) != 1:
        raise RuntimeError(f"Unexpected suspension solver initialization in {path}")

    http = runtime / "Experimental/IoStore/HttpClient/Private"
    http_sites = {
        "Socket.inl": (
            (b"#if defined(IAS_HTTP_SOCKET_FORWARD)", b"#if defined(IAS_HTTP_SOCKET_FORWARD) && !defined(UNREALPROSPERO_TARGET_PS5)"),
            (b"#else\n\tstatic int32 LastSocketResult() { return errno; }",
             b"#elif defined(UNREALPROSPERO_TARGET_PS5)\n\tstatic int32 LastSocketResult() { return errno; }\n#\tdefine IsSocketResult(err) (LastSocketResult() == Prospero_##err)\n#else\n\tstatic int32 LastSocketResult() { return errno; }"),
        ),
        "ConnectionPool.inl": (
            (b"\taddrinfo* Info = nullptr;", b"#if defined(UNREALPROSPERO_TARGET_PS5)\n\tconst int32 AddressCount = ResolveProsperoHost(HostName, MakeArrayView(IpAddresses));\n#else\n\taddrinfo* Info = nullptr;"),
            (b"\n\tif (AddressCount > 0)", b"\n#endif\n\n\tif (AddressCount > 0)"),
        ),
    }
    for name, replacements in http_sites.items():
        path = http / name
        original = path.read_bytes()
        data = original
        newline = b"\r\n" if b"\r\n" in data else b"\n"
        for old_code, new_code in replacements:
            old_code = old_code.replace(b"\n", newline)
            new_code = new_code.replace(b"\n", newline)
            if data.count(new_code) == 1:
                continue
            if data.count(old_code) != 1:
                raise RuntimeError(f"Unexpected IoStore HTTP code in {path}")
            data = data.replace(old_code, new_code)
        if data != original:
            changes[path] = data

    sockets = runtime / "Sockets/Private/BSDSockets"
    socket_calls = {
        "SocketsBSD.cpp": {
            "shutdown": "Shutdown", "bind": "Bind", "connect": "Connect", "listen": "Listen",
            "accept": "Accept", "sendto": "SendTo", "send": "Send", "recvfrom": "ReceiveFrom",
            "recv": "Receive", "getsockname": "GetSocketName", "getpeername": "GetPeerName",
            "setsockopt": "SetSocketOption", "getsockopt": "GetSocketOption", "ioctlsocket": "SocketIoctl",
            "poll": "Poll",
        },
        "SocketSubsystemBSD.cpp": {
            "socket": "OpenSocket", "getaddrinfo": "ResolveAddress", "freeaddrinfo": "FreeAddresses",
            "getnameinfo": "ResolveName", "inet_pton": "ParseInternetAddress", "poll": "Poll",
            "gethostname": "ReadHostName",
        },
        "IPAddressBSD.cpp": {"getnameinfo": "ResolveName"},
    }
    for name, calls in socket_calls.items():
        path = sockets / name
        original = path.read_bytes()
        marker = b'#include "UnrealProspero/SocketApi.h"'
        if marker in original:
            continue
        newline = b"\r\n" if b"\r\n" in original else b"\n"
        data = original.replace(b"\r\n", b"\n")
        include = b'\n#if PLATFORM_HAS_BSD_SOCKETS || PLATFORM_HAS_BSD_IPV6_SOCKETS\n'
        if data.count(include) != 1:
            raise RuntimeError(f"Unexpected BSD socket guard in {path}")
        mappings = b"\n".join(b"#define PROSPERO_SOCKET_" + source.encode() + b" UnrealProspero::Network::" + target.encode() for source, target in calls.items())
        defaults = b"\n".join(b"#define PROSPERO_SOCKET_" + source.encode() + b" " + source.encode() for source in calls)
        undefs = b"\n".join(b"#undef PROSPERO_SOCKET_" + source.encode() for source in calls)
        for source in calls:
            pattern = rb"(?m)^([ \t]*(?!//)[^\n\"]*?)\b" + source.encode() + rb"\("
            data, count = re.subn(pattern, lambda match: match[1] + b"PROSPERO_SOCKET_" + source.encode() + b"(", data)
            if count == 0:
                raise RuntimeError(f"Missing BSD socket call {source} in {path}")
        after_includes = {
            "SocketsBSD.cpp": b'#include "BSDSockets/SocketSubsystemBSD.h"\n',
            "SocketSubsystemBSD.cpp": b"#include <errno.h>\n",
            "IPAddressBSD.cpp": include,
        }[name]
        binding = b"\n#if defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5\n" + marker + b"\n" + mappings + b"\n#else\n" + defaults + b"\n#endif\n"
        if data.count(after_includes) != 1:
            raise RuntimeError(f"Unexpected BSD socket includes in {path}")
        data = data.replace(after_includes, after_includes + binding)
        data += b"\n" + undefs + b"\n"
        data = data.replace(b"PLATFORM_HAS_BSD_SOCKET_FEATURE_POLL && USE_SOCKET_FEATURE_POLL", b"((PLATFORM_HAS_BSD_SOCKET_FEATURE_POLL && USE_SOCKET_FEATURE_POLL) || (defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5))")
        if name == "SocketsBSD.cpp":
            data = data.replace(b"#else\n\tint Flags = fcntl(Socket, F_GETFL, 0);", b"#elif defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5\n\tint Value = bIsNonBlocking;\n\treturn PROSPERO_SOCKET_ioctlsocket(Socket, FIONBIO, &Value) == 0;\n#else\n\tint Flags = fcntl(Socket, F_GETFL, 0);")
            data = data.replace(b"#ifdef IP_PKTINFO", b"#if defined(IP_PKTINFO) && !(defined(UNREALPROSPERO_TARGET_PS5) && UNREALPROSPERO_TARGET_PS5)")
            data = data.replace(b"IPPROTO_IPV6, IP_MULTICAST_LOOP,", b"IPPROTO_IPV6, IPV6_MULTICAST_LOOP,")
        changes[path] = data.replace(b"\n", newline)

    path = engine / "Shaders/Private/ShaderPrint.ush"
    data = path.read_bytes()
    old_shader_print = b"#if ES3_1_PROFILE || (COMPILER_HLSL && !COMPILER_DXC)"
    new_shader_print = old_shader_print + b" || UNREALPROSPERO_SHADER_PRINT_GLOBAL_RESOURCES"
    if data.count(new_shader_print) != 1:
        if data.count(old_shader_print) != 1:
            raise RuntimeError(f"Unexpected shader print resource selection in {path}")
        changes[path] = data.replace(old_shader_print, new_shader_print)

    apply_global_geometry(runtime, changes)

    for path, data in changes.items():
        path.write_bytes(data)


def apply_global_geometry(runtime, changes):
    def edit(name, before, after, count=1):
        path = runtime / name
        text = changes.get(path, path.read_bytes())
        newline = b"\r\n" if b"\r\n" in text else b"\n"
        before, after = (value.encode().replace(b"\n", newline) for value in (before, after))
        if text.count(after) == count:
            return
        if text.count(before) != count:
            raise RuntimeError(f"Unexpected global geometry shader source in {path}")
        changes[path] = text.replace(before, after)

    shader_map = "RenderCore/Private/GlobalShader.cpp"
    compile_pipeline = "bool FGlobalShaderType::ShouldCompilePipeline(const FShaderPipelineType* ShaderPipelineType, EShaderPlatform Platform, EShaderPermutationFlags Flags)\n{"
    edit(shader_map, compile_pipeline, compile_pipeline + """
    if (FStringView(ShaderPipelineType->GetName()).StartsWith(TEXTVIEW("UnrealProsperoGlobalGeometry")) &&
        FDataDrivenShaderPlatformInfo::GetShaderFormat(Platform) != FName(TEXT("SF_AGC_ANYPS5"))) return false;""")
    permutations = "bool FGlobalShaderType::ForEachCompiledPermutation(EShaderPlatform Platform, EShaderPermutationFlags PermutationFlags, TFunction<bool(int32)> Function)\n{"
    edit(shader_map, permutations, permutations + """
    if (GetFrequency() == SF_Geometry && GetPermutationCount() == 1 && FDataDrivenShaderPlatformInfo::GetShaderFormat(Platform) == FName(TEXT("SF_AGC_ANYPS5")))
    {
        for (TLinkedList<FShaderPipelineType*>::TIterator pipeline(FShaderPipelineType::GetTypeList()); pipeline; pipeline.Next())
        {
            if ((*pipeline)->IsGlobalTypePipeline() && (*pipeline)->ShouldOptimizeUnusedOutputs(Platform) &&
                (*pipeline)->GetStages().Contains(this) && ShouldCompilePipeline(*pipeline, Platform, PermutationFlags)) return true;
        }
    }
""")

    shader_compiler = "Engine/Private/ShaderCompiler/ShaderCompilerEditor.cpp"
    section = "FGlobalShaderMapSection* Section = GGlobalShaderMap[Platform]->FindOrAddSection(ShaderType);"
    edit(shader_compiler, section, """const FShaderType* sectionType = ShaderType;
        if (ShaderPipelineType && ShaderPipelineType->HasGeometry() &&
            ShaderPipelineType->ShouldOptimizeUnusedOutputs(Platform) &&
            FStringView(ShaderPipelineType->GetName()).StartsWith(TEXTVIEW("UnrealProsperoGlobalGeometry")))
        {
            sectionType = ShaderPipelineType->GetShader(SF_Vertex);
            check(sectionType);
        }
        FGlobalShaderMapSection* Section = GGlobalShaderMap[Platform]->FindOrAddSection(sectionType);""")

    volume_header = "Renderer/Public/VolumeRendering.h"
    edit(volume_header, '#include "GlobalShader.h"', '#include "GlobalShader.h"\n#include "DataDrivenShaderPlatformInfo.h"')
    volume_anchor = "extern ENGINE_API void RasterizeToVolumeTexture(FRHICommandList& RHICmdList, FVolumeBounds VolumeBounds);"
    edit(volume_header, volume_anchor, volume_anchor + """

ENGINE_API const FShaderPipelineType* GetUnrealProsperoVolumeGeometryPipeline();

template<typename TVertex, typename TGeometry>
TPair<TShaderRef<TVertex>, TShaderRef<TGeometry>> GetUnrealProsperoGeometryShaders(const FGlobalShaderMap* shaderMap, const FShaderPipelineType* pipeline)
{
    if (FDataDrivenShaderPlatformInfo::GetShaderFormat(shaderMap->GetPlatform()) == FName(TEXT("SF_AGC_ANYPS5")))
    {
        const auto shaders = shaderMap->GetShaderPipeline(pipeline);
        check(shaders.IsValid());
        return {shaders.GetShader<TVertex>(), shaders.GetShader<TGeometry>()};
    }
    return {shaderMap->GetShader<TVertex>(), TOptionalShaderMapRef<TGeometry>(shaderMap)};
}
""")
    volume_shader = 'IMPLEMENT_SHADER_TYPE(, FWriteToSliceGS, TEXT("/Engine/Private/TranslucentLightingShaders.usf"), TEXT("WriteToSliceMainGS"), SF_Geometry);'
    edit("Engine/Private/VolumeRendering.cpp", volume_shader, volume_shader + """
IMPLEMENT_SHADERPIPELINE_TYPE_VSGS(UnrealProsperoGlobalGeometryVolume, FWriteToSliceVS, FWriteToSliceGS, true)

const FShaderPipelineType* GetUnrealProsperoVolumeGeometryPipeline()
{
    return &UnrealProsperoGlobalGeometryVolume;
}
""")
    fog = "Renderer/Private/VolumetricFog.cpp"
    fog_vertex = 'IMPLEMENT_GLOBAL_SHADER(FWriteToBoundingSphereVS, "/Engine/Private/VolumetricFog.usf", "WriteToBoundingSphereVS", SF_Vertex);'
    def deferred_pipeline(name, vertex, geometry):
        return f"""

static const FShaderPipelineType* Get{name}()
{{
    IMPLEMENT_SHADERPIPELINE_TYPE_VSGS({name}, {vertex}, {geometry}, true)
    return &{name};
}}

static FDelayedAutoRegisterHelper Register{name}(EDelayedRegisterRunPhase::PreRHIInit, [] {{ Get{name}(); }});
"""

    edit(fog, '#include "VolumeRendering.h"', '#include "VolumeRendering.h"\n#include "Misc/DelayedAutoRegister.h"')
    edit(fog, fog_vertex, fog_vertex + deferred_pipeline("UnrealProsperoGlobalGeometryFog", "FWriteToBoundingSphereVS", "FWriteToSliceGS"))
    for shader_map_name, indent in (("View.ShaderMap", "\t\t\t\t"), ("GlobalShaderMap", "\t\t")):
        edit(fog, f"auto VertexShader = {shader_map_name}->GetShader< FWriteToBoundingSphereVS >();\n{indent}TOptionalShaderMapRef<FWriteToSliceGS> GeometryShader({shader_map_name});",
             f"auto [VertexShader, GeometryShader] = GetUnrealProsperoGeometryShaders<FWriteToBoundingSphereVS, FWriteToSliceGS>({shader_map_name}, GetUnrealProsperoGlobalGeometryFog());")
    lighting = "Renderer/Private/TranslucentLighting.cpp"
    for shader_map_name, indent in (("View.ShaderMap", "\t\t\t\t\t"), ("View.ShaderMap", "\t\t\t\t"), ("View.ShaderMap", "\t\t\t\t\t\t"), ("GlobalShaderMap", "\t")):
        edit(lighting, f"\n{indent}TShaderMapRef<FWriteToSliceVS> VertexShader({shader_map_name});\n{indent}TOptionalShaderMapRef<FWriteToSliceGS> GeometryShader({shader_map_name});",
             f"\n{indent}auto [VertexShader, GeometryShader] = GetUnrealProsperoGeometryShaders<FWriteToSliceVS, FWriteToSliceGS>({shader_map_name}, GetUnrealProsperoVolumeGeometryPipeline());")
    edit(lighting, "TShaderMapRef<FWriteToSliceVS> VertexShader,\n\tTOptionalShaderMapRef<FWriteToSliceGS> GeometryShader,",
         "TShaderRef<FWriteToSliceVS> VertexShader,\n\tTShaderRef<FWriteToSliceGS> GeometryShader,", 2)
    shadows = "Renderer/Private/ShadowDepthRendering.cpp"
    cube_shader = 'IMPLEMENT_GLOBAL_SHADER(FCopyShadowMapsCubeGS, "/Engine/Private/CopyShadowMaps.usf", "CopyCubeDepthGS", SF_Geometry);'
    edit(shadows, cube_shader, cube_shader + deferred_pipeline("UnrealProsperoGlobalGeometryCube", "TScreenVSForGS<false>", "FCopyShadowMapsCubeGS"))
    include = '#include "ScreenRendering.h"'
    edit(shadows, include, include + '\n#include "VolumeRendering.h"\n#include "Misc/DelayedAutoRegister.h"')
    edit(shadows, "TShaderMapRef<TScreenVSForGS<false>> VertexShader(View.ShaderMap);\n\t\t\t\t\tTShaderMapRef<FCopyShadowMapsCubeGS> GeometryShader(View.ShaderMap);",
         "auto [VertexShader, GeometryShader] = GetUnrealProsperoGeometryShaders<TScreenVSForGS<false>, FCopyShadowMapsCubeGS>(View.ShaderMap, GetUnrealProsperoGlobalGeometryCube());")

    voxelization = "Renderer/Private/VolumetricFogVoxelization.cpp"
    edit(voxelization, '#include "VolumetricFog.h"', '#include "VolumetricFog.h"\n#include "DataDrivenShaderPlatformInfo.h"')
    pixel_shader = 'IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVoxelizeVolumePS<VMode_Object_Box>,TEXT("/Engine/Private/VolumetricFogVoxelization.usf"),TEXT("VoxelizePS"),SF_Pixel);'
    edit(voxelization, pixel_shader, pixel_shader + """

IMPLEMENT_SHADERPIPELINE_TYPE_VSGSPS(UnrealProsperoVoxelizeSphere, TVoxelizeVolumeVS<VMode_Primitive_Sphere>, TVoxelizeVolumeGS<VMode_Primitive_Sphere>, TVoxelizeVolumePS<VMode_Primitive_Sphere>, true)
IMPLEMENT_SHADERPIPELINE_TYPE_VSGSPS(UnrealProsperoVoxelizeBox, TVoxelizeVolumeVS<VMode_Object_Box>, TVoxelizeVolumeGS<VMode_Object_Box>, TVoxelizeVolumePS<VMode_Object_Box>, true)
""")
    material_shaders = "FMaterialShaders Shaders;\n\tif (!Material.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))"
    edit(voxelization, material_shaders, """if (FDataDrivenShaderPlatformInfo::GetShaderFormat(GShaderPlatformForFeatureLevel[FeatureLevel]) == FName(TEXT("SF_AGC_ANYPS5")))
    {
        ShaderTypes.PipelineType = bUsePrimitiveSphere ? &UnrealProsperoVoxelizeSphere : &UnrealProsperoVoxelizeBox;
    }
    """ + material_shaders)

    material_shader = "Engine/Private/Materials/MaterialShader.cpp"
    pipeline_filter = """\t\tif (ExcludeNonPipelinedShaderTypes(ShaderPlatform))
\t\t{
\t\t\tfor (FShaderPipelineType* Pipeline : Pipelines)
\t\t\t{
\t\t\t\tif (Pipeline->ShouldOptimizeUnusedOutputs(ShaderPlatform))
\t\t\t\t{
\t\t\t\t\tPipelinedShaderTypes.Append(Pipeline->GetStages());
\t\t\t\t}
\t\t\t}

\t\t\tbAnyTypesExcluded = PipelinedShaderTypes.Num() != 0;
\t\t}"""
    edit(material_shader, pipeline_filter, """        const bool excludePipelinedTypes = ExcludeNonPipelinedShaderTypes(ShaderPlatform);
        const bool isProspero = FDataDrivenShaderPlatformInfo::GetShaderFormat(ShaderPlatform) == FName(TEXT("SF_AGC_ANYPS5"));
        for (FShaderPipelineType* pipeline : Pipelines)
        {
            const bool isProsperoVoxelization = isProspero && FStringView(pipeline->GetName()).StartsWith(TEXTVIEW("UnrealProsperoVoxelize"));
            if ((excludePipelinedTypes || isProsperoVoxelization) && pipeline->ShouldOptimizeUnusedOutputs(ShaderPlatform))
            {
                PipelinedShaderTypes.Append(pipeline->GetStages());
            }
        }
        bAnyTypesExcluded = PipelinedShaderTypes.Num() != 0;""")
    mesh_shader = "Engine/Private/Materials/MeshMaterialShader.cpp"
    edit(mesh_shader, '#include "MeshMaterialShader.h"', '#include "MeshMaterialShader.h"\n#include "DataDrivenShaderPlatformInfo.h"')
    pipeline = "bool FMeshMaterialShaderType::ShouldCompilePipeline(const FShaderPipelineType* ShaderPipelineType, EShaderPlatform Platform, const FMaterialShaderParameters& MaterialParameters, const FVertexFactoryType* VertexFactoryType, EShaderPermutationFlags Flags)\n{"
    edit(mesh_shader, pipeline, pipeline + """
    if (FStringView(ShaderPipelineType->GetName()).StartsWith(TEXTVIEW("UnrealProsperoVoxelize")) &&
        FDataDrivenShaderPlatformInfo::GetShaderFormat(Platform) != FName(TEXT("SF_AGC_ANYPS5"))) return false;""")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("engine", type=Path)
    apply(parser.parse_args().engine)
