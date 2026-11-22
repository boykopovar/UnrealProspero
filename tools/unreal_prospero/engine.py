import json
import os
from pathlib import Path


PLATFORMS = ('AnyPS5', 'UnrealProspero')


def versions():
    path = Path(__file__).resolve().parents[1] / 'Versions.json'
    data = json.loads(path.read_text(encoding='utf-8'))
    if data['schemaVersion'] != 1:
        raise ValueError('Unsupported version manifest')
    return data


def require_engine(directory):
    directory = Path(directory).resolve(strict=True)
    path = directory / 'Engine/Build/Build.version'
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    actual = '.'.join(str(data[name]) for name in ('MajorVersion', 'MinorVersion', 'PatchVersion'))
    expected = versions()['unreal']['version']
    if actual != expected:
        raise ValueError(f'Expected Unreal Engine {expected}; found {actual}')
    for name in ('Runtime/Core/Private/HAL/ThreadingBase.cpp', 'Runtime/Engine/Private/UnrealEngine.cpp', 'Editor/UnrealEd/Private/UnrealEdEngine.cpp'):
        if not (directory / 'Engine/Source' / name).is_file():
            raise ValueError(f'Unreal Engine sources are missing: {name}')
    return directory


def canonical_path(path, strict=False):
    path = Path(path).resolve(strict=strict)
    if os.name == 'nt':
        value = str(path)
        if value.startswith('\\\\?\\UNC\\'):
            return Path('\\\\' + value[8:])
        if value.startswith('\\\\?\\') and len(path.drive) == 6 and path.drive[4].isalpha() and path.drive[5] == ':':
            return Path(value[4:])
    return path


def contained_path(root, relative):
    root = canonical_path(root, strict=True)
    relative = Path(relative)
    if relative.is_absolute() or '..' in relative.parts or not relative.parts:
        raise ValueError(f'Invalid relative path: {relative}')
    target = canonical_path(root / relative)
    if not target.is_relative_to(root):
        raise ValueError(f'Path escapes its directory: {relative}; root={root!s}; resolved={target!s}')
    return target
