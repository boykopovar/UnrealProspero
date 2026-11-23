import json
from pathlib import PurePosixPath

from .engine import contained_path
from .install import digest
from .ports import Ports


ENGINE_RUNTIME_DIRECTORIES = (
    'Engine/Platforms/UnrealProspero/Source/Runtime/BinkAudioDecoder/SDK',
    'Engine/Platforms/UnrealProspero/Source/Runtime/RadAudioCodec/SDK',
    'Engine/Platforms/UnrealProspero/Source/Runtime/OodleDataCompression/Sdks',
    'Engine/Platforms/UnrealProspero/Plugins/Compression/OodleNetwork/Sdks',
)


def is_engine_runtime_file(name):
    path = PurePosixPath(name)
    return any(path.is_relative_to(directory) for directory in ENGINE_RUNTIME_DIRECTORIES)


def verify_distributed_files(names):
    forbidden = sorted(name for name in names if is_engine_runtime_file(name))
    if forbidden:
        raise ValueError('UE runtime components must be prepared from the user installation: ' + ', '.join(forbidden))


def prepare_runtime_ports(engine, sdk, dependencies, work, cmake, jobs):
    inputs = (
        'Engine/Source/Runtime/BinkAudioDecoder/SDK/BinkAudio/Include/binka_ue_decode.h',
        'Engine/Source/Runtime/BinkAudioDecoder/SDK/BinkAudio/Src/binka_ue_decode.cpp',
        'Engine/Source/Runtime/RadAudioCodec/SDK/Include/rada_decode.h',
        'Engine/Source/Runtime/RadAudioCodec/SDK/Src/RadAudio/radaudio_decoder.c',
    )
    for name in inputs:
        if not (engine / name).is_file():
            raise FileNotFoundError('Required runtime component is missing from the Unreal Engine installation: ' + name)
    for directory, component in (
        ('Engine/Source/Runtime/OodleDataCompression/Sdks/2.9.16', 'core'),
        ('Engine/Plugins/Compression/OodleNetwork/Sdks/2.9.16', 'net'),
    ):
        for suffix in ('', '_dbg'):
            path = engine / directory / 'lib/Linux' / f'liboo2{component}linux64{suffix}.a'
            if not path.is_file():
                raise FileNotFoundError('Required Oodle archive is missing from the Unreal Engine installation: ' + str(path))
    output = work / 'runtime-ports'
    ports = Ports(engine, sdk, dependencies, work / 'runtime-build', output, str(cmake), jobs)
    ports.runtime_ports()
    ports.write_manifest()
    manifest = json.loads((output / 'PortsManifest.json').read_text(encoding='utf-8'))
    sources = {}
    for name, expected in manifest['files'].items():
        if not is_engine_runtime_file(name):
            raise ValueError('Unexpected locally generated runtime component: ' + name)
        source = contained_path(output, name)
        if digest(source) != expected:
            raise ValueError('Locally generated runtime component hash mismatch: ' + name)
        sources[name] = source
    if len(sources) != 6:
        raise ValueError('Expected six locally prepared UE runtime archives')
    return sources
