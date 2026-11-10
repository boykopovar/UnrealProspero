using System;
using System.Collections.Generic;
using System.Reflection;
using AnyUnreal.Automation;
using AutomationTool;
using UnrealBuildTool;

internal static class Program
{
    private static void Main()
    {
        MethodInfo initialize = typeof(Platform).GetMethod("InitializePlatforms", BindingFlags.Static | BindingFlags.NonPublic) ?? throw new MissingMethodException("AutomationTool platform discovery is unavailable");
        initialize.Invoke(null, new object[] { new HashSet<Assembly> { typeof(AnyWindowsPlatform).Assembly } });
        (UnrealTargetPlatform Platform, Type Implementation, string Extension)[] targets =
        {
            (UnrealTargetPlatform.AnyWindows, typeof(AnyWindowsPlatform), ".exe"),
            (UnrealTargetPlatform.AnyLinux, typeof(AnyLinuxPlatform), ""),
            (UnrealTargetPlatform.AnyPS5, typeof(AnyPS5Platform), ".elf")
        };
        foreach (var target in targets)
        {
            Platform platform = Platform.GetPlatform(target.Platform);
            if (platform.GetType() != target.Implementation) throw new InvalidOperationException("AutomationTool registered a placeholder platform: " + target.Platform);
            if (Platform.GetExeExtension(target.Platform) != target.Extension) throw new InvalidOperationException("Unexpected executable extension: " + target.Platform);
            if (platform.GetCookPlatform(false, false) != target.Platform.ToString()) throw new InvalidOperationException("Unexpected cook platform: " + target.Platform);
            Console.WriteLine("Discovered AutomationTool platform " + target.Platform);
        }
    }
}
