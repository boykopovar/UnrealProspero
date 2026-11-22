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
        managed = name in ('Directory.Build.props', 'Directory.Build.targets') or name.startswith('Engine/Source/Programs/') and name.endswith(('.props', '.targets', '.proto')) or name.startswith('Engine/Binaries/DotNET/EpicGames.Perforce.Native/')
        headers = name.startswith(('Engine/Source/ThirdParty/', 'Engine/Plugins/')) and name.endswith(('.h', '.hpp', '.hxx', '.inl', '.inc', '.ipp'))
        sources = name.startswith(('Engine/Source/ThirdParty/', 'Engine/Plugins/')) and name.endswith(('.c', '.cpp', '.cc', '.cxx', '.S', '.s', '.tar.gz', '.tgz', '.tar.xz', '.zip'))
        windows = name.startswith(('Engine/Source/ThirdParty/', 'Engine/Plugins/')) and name.endswith(('.lib', '.dll')) and any(part in name for part in ('/Win64/', '/Windows/', '/win-x64/'))
        if managed or options.group in ('headers', 'sources', 'windows-tools') and headers or options.group in ('sources', 'windows-tools') and sources or options.group == 'windows-tools' and windows:
            selected.append(name)
    if not selected:
        raise ValueError('No Unreal dependencies matched the requested group')
    dependencies.restore(selected)


if __name__ == '__main__':
    main()
