import argparse
from pathlib import Path

from unreal_prospero.dependencies import Dependencies
from unreal_prospero.engine import require_engine


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--cache', required=True, type=Path)
    parser.add_argument('--group', required=True, choices=['managed', 'headers', 'sources', 'windows-tools'])
    options = parser.parse_args()
    engine = require_engine(options.engine)
    dependencies = Dependencies(engine, options.cache)
    selected = []
    for name in dependencies.files:
        vendor = name.startswith(('Engine/Source/ThirdParty/', 'Engine/Plugins/', 'Engine/Source/Runtime/BinkAudioDecoder/SDK/', 'Engine/Source/Runtime/RadAudioCodec/SDK/', 'Engine/Source/Runtime/OodleDataCompression/Sdks/'))
        managed = name in ('Directory.Build.props', 'Directory.Build.targets') or name.startswith('Engine/Source/Programs/') and name.endswith(('.props', '.targets', '.proto')) or name.startswith('Engine/Binaries/DotNET/EpicGames.Perforce.Native/')
        headers = vendor and name.endswith(('.h', '.hh', '.hpp', '.hxx', '.inl', '.inc', '.ipp'))
        sources = vendor and name.endswith(('.c', '.cpp', '.cc', '.cxx', '.S', '.s', '.cmake', '.txt', '.in', '.raw', '.ac', '.am', '.def'))
        oodle = name in (
            'Engine/Source/Runtime/OodleDataCompression/Sdks/2.9.16/lib/Linux/liboo2corelinux64.a',
            'Engine/Source/Runtime/OodleDataCompression/Sdks/2.9.16/lib/Linux/liboo2corelinux64_dbg.a',
            'Engine/Plugins/Compression/OodleNetwork/Sdks/2.9.16/lib/Linux/liboo2netlinux64.a',
            'Engine/Plugins/Compression/OodleNetwork/Sdks/2.9.16/lib/Linux/liboo2netlinux64_dbg.a'
        )
        windows = name.startswith(('Engine/Source/ThirdParty/', 'Engine/Plugins/')) and name.endswith(('.lib', '.dll')) and any(part in name for part in ('/Win64/', '/Windows/', '/win-x64/'))
        if managed or options.group in ('headers', 'sources', 'windows-tools') and headers or options.group in ('sources', 'windows-tools') and (sources or oodle) or options.group == 'windows-tools' and windows:
            selected.append(name)
    if not selected:
        raise ValueError('No Unreal dependencies matched the requested group')
    dependencies.restore(selected)


if __name__ == '__main__':
    main()
