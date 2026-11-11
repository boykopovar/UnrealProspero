using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnrealProspero.Build.Sdk;

namespace UnrealBuildTool;

public static class UnrealProsperoReceipt
{
    private const string prefix = "UnrealProspero.";

    public static void Write(ICollection<ReceiptProperty> properties, SdkInstallation sdk)
    {
        if (properties.Any(property => property.Name.StartsWith(prefix, StringComparison.Ordinal))) throw new InvalidDataException("Receipt already contains UnrealProspero SDK metadata");
        properties.Add(new ReceiptProperty(prefix + "SdkRoot", sdk.Root));
        properties.Add(new ReceiptProperty(prefix + "Dependencies", sdk.Dependencies));
        properties.Add(new ReceiptProperty(prefix + "AnyPs5Version", sdk.AnyPs5Version));
        properties.Add(new ReceiptProperty(prefix + "CxxVersion", sdk.CxxVersion));
        properties.Add(new ReceiptProperty(prefix + "Compiler", sdk.Compiler));
    }

    public static SdkInstallation Read(IEnumerable<ReceiptProperty> properties)
    {
        Dictionary<string, string> values = new(StringComparer.Ordinal);
        foreach (ReceiptProperty property in properties)
        {
            if (!property.Name.StartsWith(prefix, StringComparison.Ordinal)) continue;
            if (string.IsNullOrWhiteSpace(property.Value) || !values.TryAdd(property.Name[prefix.Length..], property.Value)) throw new InvalidDataException("Invalid UnrealProspero receipt property: " + property.Name);
        }
        if (values.Count != 5) throw new InvalidDataException("Unexpected UnrealProspero SDK receipt fields");
        string get(string name) => values.TryGetValue(name, out string? value) ? value : throw new InvalidDataException("Missing UnrealProspero receipt property: " + name);
        SdkInstallation sdk = new(get("SdkRoot"), get("Dependencies"));
        if (sdk.AnyPs5Version != get("AnyPs5Version") || sdk.CxxVersion != get("CxxVersion") || sdk.Compiler != get("Compiler")) throw new InvalidDataException("SDK dependencies differ from the build receipt");
        return sdk;
    }
}
