using System;
using System.IO;
using System.Linq;
using EpicGames.Core;
using UnrealBuildBase;
using UnrealBuildTool;

internal static class RhiCompileChecks
{
    public static int Add(AnyUnrealToolChain toolchain, CppCompileEnvironment platformCompile, DirectoryReference intermediate, TargetMakefileBuilder graph)
    {
        CppCompileEnvironment compile = new(platformCompile);
        string runtime = Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/AnyUnreal/Source/Runtime");
        string[] modules = { "AnyUnrealShaderCommon", "AnyUnrealRHI", "ApplicationCore", "RHI" };
        foreach (string module in modules.Where(module => module != "RHI")) compile.UserIncludePaths.Add(new DirectoryReference(Path.Combine(runtime, module, "Public")));
        foreach (string module in new[] { "RHI", "RHICore", "RenderCore", "ApplicationCore" }) compile.SystemIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Source/Runtime", module, "Public"));
        compile.Definitions.AddRange(new[] { "ANYUNREALSHADERCOMMON_API=", "ANYUNREALRHI_API=", "RHI_API=", "RHICORE_API=", "RENDERCORE_API=", "APPLICATIONCORE_API=" });
        int count = 0;
        foreach (string module in modules)
        {
            FileItem[] sources = Directory.GetFiles(Path.Combine(runtime, module, "Private"), "*.cpp", SearchOption.AllDirectories).OrderBy(path => path, StringComparer.Ordinal).Select(FileItem.GetItemByPath).ToArray();
            if (sources.Length == 0) throw new InvalidDataException("RHI module contains no source files: " + module);
            if (sources.Select(source => Path.GetFileName(source.AbsolutePath)).Distinct(StringComparer.OrdinalIgnoreCase).Count() != sources.Length) throw new InvalidDataException("RHI source filenames would collide in UBT object outputs: " + module);
            CppCompileEnvironment moduleCompile = new(compile);
            if (module == "AnyUnrealRHI") moduleCompile.Definitions.Add("ANYUNREAL_WITH_UE_TRACKING=1");
            moduleCompile.Definitions.Add("UE_MODULE_NAME=\"" + module + "\"");
            CPPOutput output = toolchain.CompileAllCPPFiles(moduleCompile, sources, DirectoryReference.Combine(intermediate, "rhi", module), module, graph);
            if (output.ObjectFiles.Count != sources.Length) throw new InvalidDataException("RHI source files were omitted from compilation");
            count += sources.Length;
        }
        return count + addTrackingConfigurations(toolchain, compile, runtime, intermediate, graph);
    }

    private static int addTrackingConfigurations(AnyUnrealToolChain toolchain, CppCompileEnvironment compile, string runtime, DirectoryReference intermediate, TargetMakefileBuilder graph)
    {
        string sourceRoot = Path.Combine(runtime, "AnyUnrealRHI/Private/AnyUnreal/RHI");
        FileItem[] sources = new[] { "Memory/BufferAllocation.cpp", "Device/DynamicRHI.cpp", "Device/Module.cpp" }.Select(path => FileItem.GetItemByPath(Path.Combine(sourceRoot, path))).ToArray();
        int count = 0;
        foreach (var configuration in new[] { (Name: "off", Llm: false, Trace: false), (Name: "llm", Llm: true, Trace: false), (Name: "trace", Llm: false, Trace: true), (Name: "both", Llm: true, Trace: true) })
        {
            CppCompileEnvironment trackingCompile = new(compile);
            trackingCompile.Definitions.RemoveAll(definition => definition.StartsWith("LLM_ENABLED_IN_CONFIG=", StringComparison.Ordinal) || definition.StartsWith("UE_MEMORY_TRACE_ENABLED=", StringComparison.Ordinal) || definition.StartsWith("UE_TRACE_ENABLED=", StringComparison.Ordinal));
            trackingCompile.Definitions.Add("ANYUNREAL_WITH_UE_TRACKING=1");
            trackingCompile.Definitions.Add("UE_MODULE_NAME=\"AnyUnrealRHI\"");
            trackingCompile.Definitions.Add("LLM_ENABLED_IN_CONFIG=" + (configuration.Llm ? "1" : "0"));
            trackingCompile.Definitions.Add("UE_MEMORY_TRACE_ENABLED=" + (configuration.Trace ? "1" : "0"));
            trackingCompile.Definitions.Add("UE_TRACE_ENABLED=" + (configuration.Trace ? "1" : "0"));
            CPPOutput output = toolchain.CompileAllCPPFiles(trackingCompile, sources, DirectoryReference.Combine(intermediate, "rhi", "tracking", configuration.Name), "AnyUnrealRHI", graph);
            if (output.ObjectFiles.Count != sources.Length) throw new InvalidDataException("Memory tracking configuration omitted RHI source files: " + configuration.Name);
            count += sources.Length;
        }
        return count;
    }
}
