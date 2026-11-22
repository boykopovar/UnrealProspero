import argparse
import os
from pathlib import Path
import sys

from unreal_prospero.dependencies import Dependencies
from unreal_prospero.engine import require_engine


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--cache', required=True, type=Path)
    parser.add_argument('--group', required=True, choices=['managed', 'headers', 'sources', 'windows-tools'])
    parser.add_argument('--host-tools', action='store_true')
    parser.add_argument('--discard-cache', action='store_true')
    options = parser.parse_args()
    engine = require_engine(options.engine)
    dependencies = Dependencies(engine, options.cache, keep_cache=not options.discard_cache)
    if options.host_tools and os.name != 'nt' and sys.platform != 'linux':
        raise ValueError('The editor host must be Windows or Linux')
    host = 'Windows' if os.name == 'nt' else 'Linux'
    host_binary = 'Win64' if os.name == 'nt' else 'Linux'
    astc = 'astcenc_thunk_win64_' if os.name == 'nt' else 'libastcenc_thunk_linux64_'
    uba = 'Win64/UnrealBuildAccelerator/x64/' if os.name == 'nt' else 'Linux/UnrealBuildAccelerator/'
    selected = []
    for name in dependencies.files:
        vendor = name.startswith(('Engine/Source/ThirdParty/', 'Engine/Plugins/', 'Engine/Source/Runtime/BinkAudioDecoder/SDK/', 'Engine/Source/Runtime/RadAudioCodec/SDK/', 'Engine/Source/Runtime/OodleDataCompression/Sdks/'))
        managed = name in ('Directory.Build.props', 'Directory.Build.targets', 'Engine/Binaries/ThirdParty/fastJSON/netstandard2.0/fastJSON.dll') or name.startswith('Engine/Source/Programs/') and name.endswith(('.props', '.targets', '.proto')) or name.startswith('Engine/Binaries/DotNET/EpicGames.Perforce.Native/')
        headers = vendor and name.endswith(('.h', '.hh', '.hpp', '.hxx', '.inl', '.inc', '.ipp'))
        sources = vendor and name.endswith(('.c', '.cpp', '.cc', '.cxx', '.S', '.s', '.cmake', '.txt', '.in', '.raw', '.ac', '.am', '.def'))
        oodle = name in (
            'Engine/Source/Runtime/OodleDataCompression/Sdks/2.9.16/lib/Linux/liboo2corelinux64.a',
            'Engine/Source/Runtime/OodleDataCompression/Sdks/2.9.16/lib/Linux/liboo2corelinux64_dbg.a',
            'Engine/Plugins/Compression/OodleNetwork/Sdks/2.9.16/lib/Linux/liboo2netlinux64.a',
            'Engine/Plugins/Compression/OodleNetwork/Sdks/2.9.16/lib/Linux/liboo2netlinux64_dbg.a'
        )
        windows = name.startswith(('Engine/Source/ThirdParty/', 'Engine/Plugins/')) and (name.lower().endswith('.lib') or name.lower().endswith('.dll') and any(part in name for part in ('/Win64/', '/Windows/', '/win-x64/')))
        native_runtime = name.startswith('Engine/Binaries/ThirdParty/FBX/') and '/' + host_binary + '/' in name and (name.endswith('.dll') if os.name == 'nt' else '.so' in Path(name).suffixes) or name.startswith('Engine/Source/ThirdParty/astcenc/Thunks/' + astc)
        editor_resources = name.startswith('Engine/Binaries/ThirdParty/CEF3/' + host_binary + '/') and '/Resources/' in name or name.startswith('Engine/Binaries/ThirdParty/svn/' + host_binary + '/')
        editor_libraries = name.startswith('Engine/Plugins/Interchange/Runtime/Source/ThirdParty/Draco/lib/' + host_binary + '/') or name.startswith('Engine/Source/ThirdParty/Boost/') and ('/x64/lib/' in name if os.name == 'nt' else '/Unix/x86_64-unknown-linux-gnu/lib/' in name)
        texture_codec = name.startswith('Engine/Source/Developer/TextureFormatOodle/Sdks/') and ('/redist/' + host_binary + '/' in name or '/include/' in name)
        shared_library = (vendor or name.startswith('Engine/Binaries/ThirdParty/')) and (name.endswith('.dll') if os.name == 'nt' else '.so' in Path(name).suffixes) and (any(part in name for part in ('/Win64/', '/Windows/', '/win-x64/')) or '-Win64-' in name if os.name == 'nt' else any(part in name for part in ('/Linux/', '/Unix/x86_64-unknown-linux-gnu/', '/linux-x86_64/')) or '-Linux-' in name)
        tools = options.host_tools and (native_runtime or shared_library or editor_resources or editor_libraries or texture_codec or name.startswith('Engine/Source/ThirdParty/Intel/ISPC/bin/' + host + '/') or name.startswith('Engine/Binaries/' + uba) and not name.endswith(('.pdb', '.debug', '.sym')))
        if tools or managed or options.group in ('headers', 'sources', 'windows-tools') and headers or options.group in ('sources', 'windows-tools') and (sources or oodle) or options.group == 'windows-tools' and windows:
            selected.append(name)
    if not selected:
        raise ValueError('No Unreal dependencies matched the requested group')
    dependencies.restore(selected)


if __name__ == '__main__':
    main()
