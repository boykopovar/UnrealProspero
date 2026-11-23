import hashlib
import json
from pathlib import Path
import shutil
import subprocess
from uuid import uuid4
from xml.etree import ElementTree

from .engine import PLATFORMS, contained_path


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def source_files(root, source_checkout):
    root = Path(root).resolve(strict=True)
    if source_checkout:
        names = subprocess.check_output(['git', '-C', str(root), 'ls-files', 'Engine/Platforms'], text=True).splitlines()
        result = {name: digest(contained_path(root, name)) for name in names}
    else:
        manifest = json.loads((root / 'PackageManifest.json').read_text(encoding='utf-8'))
        if manifest['schemaVersion'] != 1:
            raise ValueError('Unsupported extension package manifest')
        result = manifest['files']
    if not result:
        raise ValueError('Extension package contains no files')
    for name, expected in result.items():
        path = contained_path(root, name)
        if not path.is_file() or digest(path) != expected:
            raise ValueError(f'Extension package hash mismatch: {name}')
    return result


def platform_destination(engine, name):
    path = contained_path(engine, name)
    relative = path.relative_to(engine).parts
    if len(relative) < 4 or relative[:2] != ('Engine', 'Platforms') or relative[2] not in PLATFORMS:
        raise ValueError(f'Package file is outside the extension platforms: {name}')
    return path


def write_state(path, files, pending_files):
    path.parent.mkdir(parents=True, exist_ok=True)
    pending = path.with_name(path.name + '.' + uuid4().hex + '.install')
    try:
        pending.write_text(json.dumps({'schemaVersion': 1, 'files': files, 'pending': pending_files}, indent=2) + '\n', encoding='utf-8')
        pending.replace(path)
    finally:
        pending.unlink(missing_ok=True)


def install_files(root, engine, files, additional_sources=None):
    root = Path(root).resolve(strict=True)
    engine = Path(engine).resolve(strict=True)
    sources = {name: contained_path(root, name) for name in files}
    files = dict(files)
    if additional_sources is not None:
        for name, source in additional_sources.items():
            if name in sources:
                raise ValueError(f'Locally generated component conflicts with the package: {name}')
            sources[name] = Path(source).resolve(strict=True)
            files[name] = digest(sources[name])
    state = engine / 'Engine/Platforms/UnrealProspero/InstallManifest.json'
    previous = {}
    interrupted = {}
    if state.exists():
        document = json.loads(state.read_text(encoding='utf-8'))
        if document['schemaVersion'] != 1:
            raise ValueError('Unsupported installed extension manifest')
        previous = document['files']
        interrupted = document['pending']
    current = {}
    for name in previous.keys() | interrupted.keys():
        path = platform_destination(engine, name)
        if path.exists():
            expected = {collection[name] for collection in (previous, interrupted) if name in collection}
            if not path.is_file() or digest(path) not in expected:
                raise ValueError(f'Installed extension file was modified: {name}')
            current[name] = digest(path)
    for name, expected in files.items():
        path = platform_destination(engine, name)
        if path.exists() and name not in current and (not path.is_file() or digest(path) != expected):
            raise ValueError(f'Existing file is not owned by this installer: {name}')
    write_state(state, current, files)
    for name in sorted(current.keys() - files.keys()):
        path = platform_destination(engine, name)
        if path.exists():
            path.unlink()
    for name, expected in files.items():
        destination = platform_destination(engine, name)
        if destination.is_file() and digest(destination) == expected:
            continue
        destination.parent.mkdir(parents=True, exist_ok=True)
        pending = destination.with_name(destination.name + '.' + uuid4().hex + '.install')
        try:
            shutil.copy2(sources[name], pending)
            if digest(pending) != expected:
                raise ValueError(f'Copied extension file hash mismatch: {name}')
            pending.replace(destination)
        finally:
            pending.unlink(missing_ok=True)
    write_state(state, files, {})


def configure_sdk(engine, sdk, dependencies, cmake):
    namespace = 'https://www.unrealengine.com/BuildConfiguration'
    ElementTree.register_namespace('', namespace)
    path = Path(engine) / 'Engine/Saved/UnrealBuildTool/BuildConfiguration.xml'
    if path.exists():
        parser = ElementTree.XMLParser(target=ElementTree.TreeBuilder(insert_comments=True))
        root = ElementTree.parse(path, parser=parser).getroot()
        if root.tag != '{' + namespace + '}Configuration':
            raise ValueError('Unexpected Unreal build configuration format')
    else:
        root = ElementTree.Element('{' + namespace + '}Configuration')
    sections = root.findall('{' + namespace + '}UnrealProspero')
    if len(sections) > 1:
        raise ValueError('Duplicate UnrealProspero build configuration')
    section = sections[0] if sections else ElementTree.SubElement(root, '{' + namespace + '}UnrealProspero')
    for name, value in (('SdkRoot', sdk), ('Dependencies', dependencies), ('CMake', cmake)):
        nodes = section.findall('{' + namespace + '}' + name)
        if len(nodes) > 1:
            raise ValueError(f'Duplicate UnrealProspero setting: {name}')
        node = nodes[0] if nodes else ElementTree.SubElement(section, '{' + namespace + '}' + name)
        node.text = str(Path(value).resolve(strict=True))
    ElementTree.indent(root, space='    ')
    path.parent.mkdir(parents=True, exist_ok=True)
    pending = path.with_name(path.name + '.' + uuid4().hex + '.install')
    try:
        ElementTree.ElementTree(root).write(pending, encoding='utf-8', xml_declaration=True)
        pending.replace(path)
    finally:
        pending.unlink(missing_ok=True)
